#ifndef MEMOIR_NOELLEWRAPPER_H
#define MEMOIR_NOELLEWRAPPER_H
#pragma once

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "memoir/analysis/DependenceAnalysis.hpp"
#include "noelle/core/Noelle.hpp"

namespace llvm::memoir {

class NoelleWrapper : public ModulePass {
  Noelle *noelle;
  memoir::DependenceAnalysis dependenceAnalysis;

public:
  static char ID;

  NoelleWrapper();

  Noelle &getNoelle();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

} // namespace llvm::memoir

#endif
