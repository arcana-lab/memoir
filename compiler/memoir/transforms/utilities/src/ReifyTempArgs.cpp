// LLVM
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/Cloning.h"

// MEMOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/utility/Metadata.hpp"

#include "memoir/transforms/utilities/ReifyTempArgs.hpp"

namespace llvm::memoir {

static llvm::Function *find_tempargs_to_reify(
    llvm::Module &module,
    Set<llvm::Function *> &handled,
    Vector<llvm::LoadInst *> &temp_loads) {

  // Clear the list of temporary loads found in the last pass.
  temp_loads.clear();

  // Collect all of the loads and stores marked as temporary arguments.
  for (auto &function : module) {
    if (function.empty() or not function.hasName())
      continue;

    if (handled.contains(&function))
      continue;
    else
      handled.insert(&function);

    for (auto &inst : llvm::instructions(function)) {
      // Filter out irrelevant instruction types.
      auto *load = dyn_cast<llvm::LoadInst>(&inst);
      if (not load)
        continue;

      // Check for temporary argument metadata.
      if (not Metadata::get<TempArgumentMetadata>(inst))
        continue;

      // Save the temporary arg load.
      temp_loads.push_back(load);
    }

    if (not temp_loads.empty()) {
      return &function;
    }
  }

  return nullptr;
}

struct Patch {
  Map<llvm::CallBase *, llvm::StoreInst *> stores;
  OrderedSet<llvm::LoadInst *> loads;
  llvm::Argument *arg;

  void store(llvm::CallBase *call, llvm::StoreInst *store) {
    this->stores[call] = store;
  }

  void load(llvm::LoadInst *load) {
    this->loads.insert(load);
  }

  void remap(llvm::CallBase *old_call, llvm::CallBase *new_call) {
    // Search for the old call.
    auto found = this->stores.find(old_call);
    if (found == this->stores.end()) {
      return;
    }
    // If found, extract the element, update its key, and re-insert it.
    auto node = this->stores.extract(found);
    node.key() = new_call;
    this->stores.insert(std::move(node));
  }
};

using Patches = OrderedMap<llvm::GlobalVariable *, Patch>;

static llvm::Function &clone_function(llvm::Function &function,
                                      Patches &to_patch,
                                      Set<llvm::Value *> &to_cleanup) {

  auto &module = MEMOIR_SANITIZE(function.getParent(),
                                 "Function does not belong to a module!");

  // Collect patch information for this function.
  Vector<llvm::LoadInst *> temp_loads = {};
  for (const auto &[global, patch] : to_patch) {
    for (auto *load : patch.loads) {
      temp_loads.push_back(load);
    }
  }

  // Fetch the old function type.
  auto *old_func_type = function.getFunctionType();

  // Construct the new function type.
  auto *return_type = old_func_type->getReturnType();
  Vector<llvm::Type *> param_types(old_func_type->param_begin(),
                                   old_func_type->param_end());
  for (auto *load : temp_loads) {
    // Add the parameter type for the load.
    auto *load_type = load->getType();
    param_types.push_back(load_type);
  }
  auto *new_func_type = llvm::FunctionType::get(return_type,
                                                param_types,
                                                /* variadic? */ false);

  // Create the new function.
  auto &new_func = MEMOIR_SANITIZE(llvm::Function::Create(new_func_type,
                                                          function.getLinkage(),
                                                          function.getName(),
                                                          module),
                                   "Failed to create function clone.");

  // Clone the function into a new function.
  llvm::ValueToValueMapTy vmap;
  unsigned arg_no = 0;
  for (auto &old_arg : function.args()) {
    auto *new_arg = new_func.getArg(arg_no++);
    vmap.insert({ &old_arg, new_arg });
  }
  llvm::SmallVector<llvm::ReturnInst *, 8> returns;
  llvm::CloneFunctionInto(&new_func,
                          &function,
                          vmap,
                          llvm::CloneFunctionChangeType::LocalChangesOnly,
                          returns);

  new_func.takeName(&function);

  // Remap the patches.
  for (auto &[_global, patch] : to_patch) {
    auto &stores = patch.stores;

    // Update remapped calls in the patch.
    bool changed;
    do {
      changed = false;
      for (auto it = stores.begin(); it != stores.end(); ++it) {
        // Search for the old call in the vmap.
        auto *old_call = it->first;
        auto found = vmap.find(old_call);
        if (found == vmap.end())
          continue;

        // Extract the node.
        auto node = stores.extract(it);

        // Update the call.
        auto *new_call = cast<llvm::CallBase>(found->second);
        node.key() = new_call;

        // Re-insert the node.
        auto result = stores.insert(std::move(node));
        it = result.position;

        // Update the store.
        it->second = cast<llvm::StoreInst>(vmap[it->second]);

        changed = true;
      }
    } while (changed);
  }

  // Replace each of the old temporary arguments with its new argument. In
  // the process, create a mapping from temparg global to the formal
  // parameter.
  for (auto *load : temp_loads) {

    // Fetch the new state.
    auto *new_load = cast<llvm::LoadInst>(&*vmap[load]);
    auto *new_arg = new_func.getArg(arg_no++);

    // Save the corresponding global variable.
    auto *ptr = load->getPointerOperand();
    auto *global = dyn_cast<llvm::GlobalVariable>(ptr);
    MEMOIR_ASSERT(global,
                  "Temporary argument load has non-global pointer operand.");
    to_patch[global].arg = new_arg;

    // Replace the load with the argument.
    new_load->replaceAllUsesWith(new_arg);

    // Mark the cloned load for deletion, the original will be deleted when the
    // old function is.
    to_cleanup.insert(new_load);
  }

  to_cleanup.insert(&function);

  return new_func;
}

static void patch_retphis(llvm::CallBase &call,
                          llvm::Function &old_func,
                          llvm::Function &new_func) {
  for (auto *inst = call.getNextNode(); inst != NULL;
       inst = inst->getNextNode()) {
    if (auto *call = dyn_cast<llvm::CallBase>(inst)) {
      if (into<RetPHIInst>(call)) {
        call->replaceUsesOfWith(&old_func, &new_func);
      } else {
        break;
      }
    }
  }
}

static void cleanup(Set<llvm::Value *> &to_cleanup) {

  Set<llvm::Instruction *> to_delete = {};
  for (auto *val : to_cleanup) {
    if (auto *inst = dyn_cast<llvm::Instruction>(val)) {
      inst->eraseFromParent();
    }
  }

  for (auto *val : to_cleanup) {
    if (auto *func = dyn_cast<llvm::Function>(val)) {
      func->dropAllReferences();
      if (!func->empty())
        func->deleteBody();
      func->eraseFromParent();
    }
  }
}

static void reify_function(llvm::Function &function,
                           Vector<llvm::LoadInst *> &temp_loads) {

  auto &module =
      MEMOIR_SANITIZE(function.getParent(), "Function has no parent");
  auto &context = module.getContext();

  // Track the instructions that need to be cleaned up once we're done.
  Set<llvm::Value *> to_cleanup = {};

  // Collect the globals from each of the loads we found.
  Patches to_patch;
  for (auto *load : temp_loads) {
    auto *ptr = load->getPointerOperand();
    auto *global = dyn_cast<llvm::GlobalVariable>(ptr);
    MEMOIR_ASSERT(global, "temparg load to non-global");

    for (auto &use : global->uses()) {
      if (auto *store = dyn_cast<llvm::StoreInst>(use.getUser())) {
        if (store->getPointerOperand() == global) {

          // Find the corresponding call.
          llvm::CallBase *call = nullptr;
          for (auto *curr = store->getNextNode(); curr != nullptr;
               curr = curr->getNextNode()) {
            call = dyn_cast<llvm::CallBase>(curr);

            // Skip non-calls.
            if (not call) {
              continue;
            }

            // If this is a call to the function, we found it!
            if (call->getCalledFunction() == &function) {
              break;
            }

            // If this is a fold where the body is the function, we found it!
            if (auto *fold = into<FoldInst>(curr)) {
              if (&fold->getBody() == &function) {
                break;
              }
            }
          }
          if (not call) {
            MEMOIR_UNREACHABLE("Failed to find call for temparg.");
          }

          if (to_patch[global].stores.contains(call)) {
            MEMOIR_UNREACHABLE(
                "Found multiple temparg stores to same global for single call!");
          } else {
            to_patch[global].store(call, store);
          }
        }
      }
    }

    // If we couldn't find any stores for this load, replace the load with the
    // initializer.
    if (not to_patch.contains(global) or to_patch.at(global).stores.empty()) {
      auto *init = global->getInitializer();
      if (not init) {
        // If there is no initializer, get the undef value.
        init = llvm::UndefValue::get(load->getType());
      }

      load->replaceAllUsesWith(init);

    } else {
      to_patch[global].load(load);
    }

    to_cleanup.insert(load);
  }

  // If there is no work to be done, we're finished!
  if (to_patch.empty()) {
    cleanup(to_cleanup);
    return;
  }

  // Clone the function, converting temp args to formal parameters.
  auto &new_func = clone_function(function, to_patch, to_cleanup);

  // For each patch the arguments.
  Map<llvm::CallBase *, Vector<llvm::Value *>> new_arguments;
  for (const auto &[global, patch] : to_patch) {
    for (const auto &[call, store] : patch.stores) {

      auto new_arg_size = call->arg_size() + to_patch.size();

      // If the argument list doesn't have a closed keyword yet, add it.
      bool add_closed_keyword = false;
      if (auto *fold = into<FoldInst>(call)) {
        if (not fold->getClosed()) {
          add_closed_keyword = true;
        }
      }

      if (add_closed_keyword) {
        new_arg_size += 1;
      }

      // Only need to initialize arguments once
      if (not new_arguments.contains(call)) {

        // Find the new size of the function.
        Vector<llvm::Value *> new_args(new_arg_size, nullptr);

        // Populate the existing arguments.
        for (auto &arg_use : call->args()) {
          auto *new_arg = arg_use.get();
          if (new_arg == &function) {
            new_arg = &new_func;
          }

          new_args[arg_use.getOperandNo()] = new_arg;
        }

        if (add_closed_keyword) {
          // Find the first NULL argument.
          auto it = std::find(new_args.begin(), new_args.end(), nullptr);
          auto &closed_str = Keyword::get_llvm<ClosedKeyword>(context);
          *it = &closed_str;
        }

        new_arguments[call] = new_args;
      }

      // Fetch the new argument list.
      auto &new_args = new_arguments.at(call);

      // Determine the argument index for this store.
      auto param_idx = patch.arg->getArgNo();
      auto arg_offset =
          new_func.arg_size() - param_idx; // How far we are from the end.
      auto arg_idx = new_arg_size - arg_offset;

      // Get the value from the store.
      auto *value = store->getValueOperand();
      new_args.at(arg_idx) = value;

      // Mark the store for cleanup.
      to_cleanup.insert(store);
    }
  }

  // Rebuild each of the calls.
  for (const auto &[call, args] : new_arguments) {
    // Validate the arguments.
    bool invalid = false;
    for (auto *arg : args) {
      MEMOIR_ASSERT(arg, "Argument is NULL!");
    }

    // Rebuild the call.
    MemOIRBuilder builder(call);

    auto *callee = call->getCalledFunction();
    if (callee == &function) {
      callee = &new_func;
    }

    auto *new_call = builder.CreateCall(llvm::FunctionCallee(callee), args);

    call->replaceAllUsesWith(new_call);
    new_call->cloneDebugInfoFrom(call);

    to_cleanup.insert(call);

    // Patch return PHIs.
    patch_retphis(*call, function, new_func);
  }

  cleanup(to_cleanup);

  return;
}

bool reify_tempargs(llvm::Module &module) {
  bool changed = false;

  Set<llvm::Function *> handled = {};
  Vector<LoadInst *> temp_loads = {};
  while (auto *func = find_tempargs_to_reify(module, handled, temp_loads)) {

    if (func->getName() == "main")
      continue;

    reify_function(*func, temp_loads);

    MemOIRInst::invalidate();
  }

  return changed;
}

} // namespace llvm::memoir
