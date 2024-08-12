#include "memoir/analysis/DefUseChain.hpp"

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
  for (auto *decl : result.decls) {
    // Insert the declaration into its own chain.
    result.chains.insert(decl);

    // Add each user to the chain.
    for (auto *user : decl->users()) {
      result.chains.insert(user);
      result.chains.merge(decl, user);
    }
  }

  // Reify the def-use chains.
  result.chains.reify();

  return result;
}

llvm::AnalysisKey DefUseChainAnalysis::Key;

} // namespace llvm::memoir
