#include "memoir/ir/CallGraph.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Casting.hpp"

namespace memoir {

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

bool possible_callers(llvm::Function &function,
                      Set<llvm::CallBase *> &callers) {

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

  return possibly_unknown;
}

Set<llvm::CallBase *> possible_callers(llvm::Function &function) {
  Set<llvm::CallBase *> callers;
  possible_callers(function, callers);
  return callers;
}

llvm::CallBase *single_caller(llvm::Function &function) {

  if (is_externally_visible(function))
    return NULL;

  llvm::CallBase *single_call = NULL;
  for (auto &use : function.uses()) {
    auto *call = dyn_cast<llvm::CallBase>(use.getUser());

    // Check if the use may lead to an indirect call.
    if (not call) {
      return NULL; // Escaped!

    } else if (auto *ret_phi = into<RetPHIInst>(call)) {
      if (&use != &ret_phi->getCalledOperandAsUse())
        return NULL; // Escaped!

    } else if (auto *fold = into<FoldInst>(call)) {
      if (&use != &fold->getBodyOperandAsUse())
        return NULL; // Escaped!

    } else if (&use != &call->getCalledOperandUse()) {
      return NULL; // Escaped!

    } else if (single_call) {
      return NULL; // Multiple callers.

    } else {
      single_call = call;
    }
  }

  return single_call;
}

CallGraph::CallGraph(llvm::Module &module) : llvm::CallGraph(module) {

  llvm::CallGraph &callgraph = *this;

  // Handle external call edges.
  auto *external = callgraph.getExternalCallingNode();

  bool changed;
  do {
    // Iterate until we don't find a call edge to eliminate.
    // We do it this way because the CallGraphNode iterators are not nice.
    changed = false;

    for (const auto &[_, out] : *external) {
      auto *callee = out->getFunction();
      if (not callee) {
        continue;
      }

      // Don't remove external edges to externally visible functions.
      if (is_externally_visible(*callee)) {
        continue;
      }

      // Remove external call edges for fold bodies.
      if (FoldInst::get_single_fold(*callee)) {
        external->removeAnyCallEdgeTo(out);
        changed = true;
        continue;
      }

      // Remove external call edges due to ret phis.
      // Count the number of real, non-call uses.
      auto count = 0;
      for (auto &use : callee->uses()) {
        // If this is a direct call, or a ret phi, don't count it.
        if (auto *call = dyn_cast<llvm::CallBase>(use.getUser())) {
          if (into<RetPHIInst>(call) or &use == &call->getCalledOperandUse()) {
            continue;
          }
        }

        ++count;
      }

      // If there are no real non-call uses, remove all call edges.
      if (count == 0) {
        external->removeAnyCallEdgeTo(out);
        changed = true;
        continue;
      }
    }

  } while (changed);

  // Replace src->fold call edges with src->body call edges.
  for (auto &function : module) {
    if (auto *fold = FoldInst::get_single_fold(function)) {
      auto *fold_call = &fold->getCallInst();
      auto *caller = fold->getFunction();
      if (not caller) {
        continue;
      }

      auto *caller_node = callgraph[caller];

      // Remove the call edge for the fold.
      caller_node->removeCallEdgeFor(*fold_call);

      // Add a new call edge from the fold to the body.
      auto *body_node = callgraph[&function];
      caller_node->addCalledFunction(fold_call, body_node);
    }
  }

  // Remove all edges to memoir functions.
  for (auto &function : module) {
    if (FunctionNames::is_memoir_call(function)) {
      for (auto &[func, node] : callgraph) {
        node->removeAnyCallEdgeTo(callgraph[&function]);
      }
    }
  }
}

} // namespace memoir
