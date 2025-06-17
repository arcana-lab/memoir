#include "memoir/ir/CallGraph.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Casting.hpp"

namespace llvm::memoir {

bool is_externally_visible(llvm::Function &function) {
  return not(function.hasInternalLinkage() or function.hasPrivateLinkage());
}

llvm::CallBase *unknown_caller() {
  static llvm::CallBase *UNKNOWN = NULL;
  return UNKNOWN;
}

bool is_unknown_caller(const llvm::CallBase *caller) {
  return caller == unknown_caller();
}

bool has_unknown_caller(const Set<llvm::CallBase *> &callers) {
  return callers.contains(unknown_caller());
}

Set<llvm::CallBase *> possible_callers(llvm::Function &function) {
  Set<llvm::CallBase *> callers = {};

  bool possibly_unknown = is_externally_visible(function);

  for (auto &use : function.uses()) {
    auto *call = dyn_cast<llvm::CallBase>(use.getUser());

    // Check if the use may lead to an indirect call.
    if (not call) {
      possibly_unknown = true;

    } else if (auto *ret_phi = into<RetPHIInst>(call)) {
      if (&use != &ret_phi->getCalledOperandAsUse()) {
        possibly_unknown = true;
      }

    } else if (auto *fold = into<FoldInst>(call)) {
      if (&use != &fold->getBodyOperandAsUse()) {
        possibly_unknown = true;
      }

    } else if (&use != &call->getCalledOperandUse()) {
      possibly_unknown = true;

    } else {
      // Otherwise, we have a direct call to the function.
      callers.insert(call);
    }
  }

  // If there are possible unknown callers, the add the NULL caller
  if (possibly_unknown) {
    callers.insert(unknown_caller());
  }

  return callers;
}

} // namespace llvm::memoir
