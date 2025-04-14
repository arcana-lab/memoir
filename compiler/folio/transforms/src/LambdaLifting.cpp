#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/TypeCheck.hpp"
#include "memoir/support/Casting.hpp"

#include "folio/transforms/LambdaLifting.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::Use &get_called_use(llvm::CallBase &call) {
  auto *fold = into<FoldInst>(call);

  return fold ? fold->getBodyOperandAsUse() : call.getCalledOperandUse();
}

/**
 * Get the number of 'real' uses of the function, not including RetPHIs.
 */
static unsigned num_real_uses(llvm::Function &func) {
  unsigned count = 0;
  for (auto &use : func.uses()) {
    auto *user = use.getUser();
    if (not into<RetPHIInst>(user)) {
      ++count;
    }
  }
  return count;
}

static void patch_ret_phis(llvm::Instruction &I, llvm::Function &F) {
  auto *next = I.getNextNode();
  while (next) {

    if (auto *ret_phi = into<RetPHIInst>(next)) {
      ret_phi->getCalledOperandAsUse().set(&F);
    } else {
      // Stop once we see a non-RetPHI.
      break;
    }

    next = next->getNextNode();
  }
  return;
}

static void queue_new_calls(llvm::Function &F,
                            Vector<llvm::CallBase *> &calls,
                            const Set<llvm::Function *> &memoir_functions) {
  for (auto &BB : F) {
    for (auto &I : BB) {
      auto *new_call = dyn_cast<llvm::CallBase>(&I);
      if (not new_call) {
        continue;
      }

      auto &new_use = get_called_use(*new_call);
      auto *new_func = dyn_cast<llvm::Function>(new_use.get());
      if (not new_func) {
        continue;
      }

      if (memoir_functions.count(new_func) > 0) {
        calls.push_back(new_call);
      }
    }
  }
}

LambdaLifting::LambdaLifting(llvm::Module &M) : M(M) {

  // Identify functions that have more than one callers.
  Set<llvm::Function *> memoir_functions = {};
  for (auto &F : M) {

    if (F.empty()) {
      continue;
    }

    // Check if any of the arguments are memoir collections.
    bool has_memoir = false;
    for (auto &arg : F.args()) {

      // Get the type of the argument.
      auto *type = type_of(arg);

      // If it is a collection type, we will need to handle this function.
      if (isa_and_nonnull<CollectionType>(type)) {
        has_memoir = true;
        break;
      }
    }

    // Skip functions that have no memoir arguments.
    if (not has_memoir) {
      continue;
    }

    // Save the function.
    memoir_functions.insert(&F);
  }

  // Collect all the calls to this function.
  Map<llvm::Function *, Vector<llvm::CallBase *>> recursive = {};
  Vector<llvm::CallBase *> calls = {};
  for (auto *func : memoir_functions) {
    for (auto &use : func->uses()) {

      // Only handle call users.
      auto *call = dyn_cast<llvm::CallBase>(use.getUser());
      if (not call) {
        continue;
      }

      // Skip non-fold memoir instructions.
      auto *fold = into<FoldInst>(call);
      if (not fold and into<MemOIRInst>(call)) {
        continue;
      }

      // Ensure that the use is the called operand.
      if (fold and use != fold->getBodyOperandAsUse()) {
        continue;
      } else if (use != call->getCalledOperandUse()) {
        continue;
      }

      if (call->getFunction() == func) {
        recursive[func].push_back(call);
      } else {
        calls.push_back(call);
      }
    }
  }

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *fold = into<FoldInst>(I);
        if (not fold) {
          continue;
        }

        if (num_real_uses(fold->getBody()) > 1) {
          MEMOIR_UNREACHABLE("Fold with non-unique body!");
        }
      }
    }
  }

  // First, convert any self-recursive function into two mutually recursive
  // functions.
  for (const auto &[func, recursive_calls] : recursive) {

    // Create a clone of the function.
    llvm::ValueToValueMapTy vmap;
    auto *clone = llvm::CloneFunction(func, vmap);

    debugln("CREATED  ", clone->getName());
    debugln("  CALLER ", func->getName());

    // Update the recursive calls in the original function to call the clone
    // (the cloned calls already call the original).
    for (auto *call : recursive_calls) {
      auto &use = get_called_use(*call);
      use.set(clone);

      // Patch up the following RetPHIs.
      patch_ret_phis(*call, *clone);

      auto *cloned_call = dyn_cast<llvm::CallBase>(&*vmap[call]);

      // calls.push_back(cloned_call);
    }
  }

  // Convert all folds to have unique bodies.
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *fold = into<FoldInst>(I);
        if (not fold) {
          continue;
        }

        auto &body = fold->getBody();
        if (num_real_uses(body) == 1) {
          continue;
        }

        // Clone the fold body.
        llvm::ValueToValueMapTy vmap;
        auto *clone = llvm::CloneFunction(&body, vmap);

        debugln("CREATED  ", clone->getName());
        debugln("  CALLER ", F.getName());

        // Set the fold's body to the clone.
        fold->getBodyOperandAsUse().set(clone);

        // Patch up the following RetPHIs.
        patch_ret_phis(I, *clone);

        // If we have created a call to any function with memoir collections,
        // add it to the worklist.
        queue_new_calls(*clone, calls, memoir_functions);

        if (memoir_functions.count(&body)) {
          memoir_functions.insert(clone);
        }
      }
    }
  }

  // For each function, create a unique clone of it for each call site.
  while (not calls.empty()) {
    // Pop a call off the worklist.
    auto *call = calls.back();
    calls.pop_back();

    // Try to cast the call to a fold.
    auto *fold = into<FoldInst>(call);

    // Fetch the callee information.
    auto &use = get_called_use(*call);
    auto *func = dyn_cast<llvm::Function>(use.get());

    // If this is the sole user of the function, we are all good.
    if (num_real_uses(*func) == 1) {
      continue;
    }

    // If this is a self-recursive function, skip it.
    if (call->getFunction() == func) {
      continue;
    }

    // Clone the function for this call site.
    llvm::ValueToValueMapTy vmap;
    auto *clone = llvm::CloneFunction(func, vmap);

    debugln("CREATED  ", clone->getName());
    debugln("  CALLER ", call->getCaller()->getName());

    // Replace the called operand with the clone function.
    use.set(clone);

    // Patch up the following RetPHIs.
    patch_ret_phis(*call, *clone);

    // If we have created a call to any function with memoir collections, add it
    // to the worklist.
    queue_new_calls(*clone, calls, memoir_functions);

    memoir_functions.insert(clone);
  }

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *fold = into<FoldInst>(I);
        if (not fold) {
          continue;
        }

        if (num_real_uses(fold->getBody()) > 1) {
          MEMOIR_UNREACHABLE("Fold with non-unique body!");
        }
      }
    }
  }
}

} // namespace folio
