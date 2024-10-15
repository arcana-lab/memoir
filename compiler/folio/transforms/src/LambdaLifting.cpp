#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/TypeCheck.hpp"
#include "memoir/support/Casting.hpp"

#include "folio/transforms/LambdaLifting.hpp"

using namespace llvm::memoir;

namespace folio {

LambdaLifting::LambdaLifting(llvm::Module &M) : M(M) {

  // Identify functions that have more than one callers.
  ordered_map<llvm::Function *, ordered_set<llvm::CallBase *>> callers = {};
  for (auto &F : M) {

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

    // Save each of the function callers.
    for (auto &use : F.uses()) {
      auto *user = use.getUser();

      if (auto *call = dyn_cast<llvm::CallBase>(user)) {
        // Skip memoir instructions.
        if (into<MemOIRInst>(call)) {
          continue;
        }

        // Ensure the use is the called operand.
        if (&use == &call->getCalledOperandUse()) {
          // Insert the caller.
          callers[&F].insert(call);
          continue;
        }
      }

      // If the use didn't fit into the above cases, we will warn the user that
      // their memoir collections are being passed into a function that _may_ be
      // indirectly called.
      warnln("Function ", F.getName(), " may be indirectly called.");
    }
  }

  // Erase all functions in the mapping with zero or one callers.
  std::erase_if(callers, [](const auto &item) {
    const auto &[func, calls] = item;
    return calls.size() <= 1;
  });

  // For each function, create a unique clone of it for each call site.
  for (auto [func, calls] : callers) {
    bool first = true;
    for (auto *call : calls) {
      // Skip the first call.
      if (first) {
        first = false;
        continue;
      }

      // Create the function clone.
      llvm::ValueToValueMapTy vmap;
      auto *clone = llvm::CloneFunction(func, vmap);

      // Replace the called operand with the clone function.
      call->getCalledOperandUse().set(clone);

      // Patch up the following RetPHIs.
      auto *next = call->getNextNode();
      while (next) {

        if (auto *ret_phi = into<RetPHIInst>(next)) {
          ret_phi->getCalledOperandAsUse().set(clone);
        } else {
          // Stop once we see a non-RetPHI.
          break;
        }

        next = next->getNextNode();
      }
    }
  }
}

} // namespace folio
