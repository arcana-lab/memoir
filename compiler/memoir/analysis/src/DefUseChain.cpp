#include "memoir/analysis/DefUseChain.hpp"

#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// DefUseChainResult
set<llvm::Value *> DefUseChainResult::declarations(llvm::Value &V) {
  set<llvm::Value *> decls;

  // Find the root of the value.
  auto *root = this->chains.find(&V);

  // Find all declarations that share the same root.
  for (auto *decl : this->decls) {
    auto *decl_root = this->chains.find(decl);

    if (decl_root == root) {
      decls.insert(decl);
    }
  }

  return decls;
}

set<llvm::Value *> DefUseChainResult::chain(llvm::Value &V) {
  set<llvm::Value *> chain = { &V };

  // Find the root of the value.
  auto *root = this->chains.find(&V);

  // Find all values in the same chain as the value.
  for (const auto &[var_root, var] : this->chains) {
    if (var_root == root) {
      chain.insert(var);
    }
  }

  return decls;
}

// DefUseChainAnalysis
static void collect_uses(UnionFind<llvm::Value *> &chains,
                         set<llvm::Value *> &visited,
                         llvm::Value &V) {
  // Check that we have not already visited this value.
  if (visited.count(&V) > 0) {
    return;
  } else {
    visited.insert(&V);
  }

  // For each user:
  for (auto &use : V.uses()) {
    // Unpack the use.
    auto *user = use.getUser();

    // If the user is not an instruction, skip it.
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (not user_as_inst) {
      continue;
    }

    // If the user is a PHI node, add it to the Def-Use chain and merge all .
    if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
      chains.find(user);
      chains.merge(&V, user);
    }

    // If the user is a MEMOIR instruction, add it to the Def-Use chain.
    else if (auto *memoir_inst = into<MemOIRInst>(user_as_inst)) {
      // Skip type instructions.
      if (isa<AssertTypeInst>(memoir_inst)) {
        continue;
      }

      // Handle basic operations.
      if (auto *write = dyn_cast<WriteInst>(memoir_inst)) {
        auto &resultant = write->getCallInst();
        chains.merge(&V, &resultant);
        collect_uses(chains, visited, resultant);
      } else if (auto *insert = dyn_cast<InsertInst>(memoir_inst)) {

        // If this is a SeqInsertSeqInst, check that the use is the base
        // collection.
        if (auto *seq_insert_seq = dyn_cast<SeqInsertSeqInst>(insert)) {
          if (use.getOperandNo()
              != seq_insert_seq->getBaseCollectionAsUse().getOperandNo()) {
            continue;
          }
        }

        auto &resultant = insert->getCallInst();
        chains.merge(&V, &resultant);
        collect_uses(chains, visited, resultant);
      } else if (auto *remove = dyn_cast<RemoveInst>(memoir_inst)) {
        auto &resultant = remove->getCallInst();
        chains.merge(&V, &resultant);
        collect_uses(chains, visited, resultant);
      } else if (auto *swap = dyn_cast<SeqSwapInst>(memoir_inst)) {
        warnln("SeqSwapInst is currently unhandled!");
      } else if (auto *swap = dyn_cast<SwapInst>(memoir_inst)) {
        auto &resultant = swap->getCallInst();
        chains.merge(&V, &resultant);
        collect_uses(chains, visited, resultant);
      } else if (auto *copy = dyn_cast<CopyInst>(memoir_inst)) {
        auto &resultant = copy->getCopy();
        chains.merge(&V, &resultant);
        collect_uses(chains, visited, resultant);
      }

      // If the instruction is a fold, and the use is the initial value, link
      // the interprocedural def-use chain.
      if (auto *fold = dyn_cast<FoldInst>(memoir_inst)) {
        // If the value is the initial value operand, add the resultant to the
        // def-use chain.
        if (&fold->getInitial() == &V) {

          auto &fold_result = fold->getResult();

          chains.find(&fold_result);
          chains.merge(&V, &fold_result);

          collect_uses(chains, visited, fold_result);

          continue;
        }

        // If the value is the collection being folded over, do nothing.
        if (&fold->getCollection() == &V) {
          continue;
        }

        // Otherwise, link with the corresponding argument.
        auto &arg = fold->getClosedArgument(use);

        chains.merge(&V, &arg);

        collect_uses(chains, visited, arg);
      }
    }

    // If the instruction is a call, link the interprocedural def-use chain.
    else if (auto *call = dyn_cast<llvm::CallBase>(user_as_inst)) {

      // Gather all arguments to unify.
      set<llvm::Argument *> arguments = {};

      // Get the called function.
      auto *called_function = call->getCalledFunction();

      // If this is an indirect call, gather all possibly called functions.
      // TODO: this could be improved by using NOELLE's complete call graph or
      // LLVM points-to analysis.
      if (not called_function) {
        auto *function_type = call->getFunctionType();

        // For each function that shares the function type.
        auto *module = user_as_inst->getModule();
        for (auto &F : *module) {
          // Check the function type.
          if (F.getFunctionType() == function_type) {

            // Get the corresponding argument.
            auto *arg = F.getArg(use.getOperandNo());

            arguments.insert(arg);
          }
        }
      } else {
        // Otherwise, this is a direct call, unify the corresponding argument.
        auto *arg = called_function->getArg(use.getOperandNo());

        arguments.insert(arg);
      }

      // For each argument, unify it with the value being inspected.
      for (auto *arg : arguments) {
        chains.merge(&V, arg);

        // Recurse on the argument.
        collect_uses(chains, visited, *arg);
      }
    }
  }

  return;
}

DefUseChainResult DefUseChainAnalysis::run(llvm::Module &M,
                                           llvm::ModuleAnalysisManager &MAM) {
  DefUseChainResult result;

  // First, gather all of the collection declarations in the program.
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *alloc = into<AllocInst>(&I)) {
          result.decls.insert(&alloc->getCallInst());
        } else if (auto *keys = into<AssocKeysInst>(&I)) {
          result.decls.insert(&keys->getCallInst());
        }
      }
    }
  }

  // Collect the def-use chains with forward analysis.
  set<llvm::Value *> visited = {};
  for (auto *decl : result.decls) {
    // Insert the declaration into its own chain.
    result.chains.insert(decl);

    // Collect all uses of the value.
    collect_uses(result.chains, visited, *decl);
  }

  // Reify the def-use chains.
  result.chains.reify();

  return result;
}

llvm::AnalysisKey DefUseChainAnalysis::Key;

} // namespace llvm::memoir
