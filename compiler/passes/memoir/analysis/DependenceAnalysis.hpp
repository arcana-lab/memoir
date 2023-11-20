#ifndef MEMOIR_DEPENDENCEANALYSIS_H
#define MEMOIR_DEPENDENCEANALYSIS_H
#pragma once

#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Print.hpp"
#include "noelle/core/Noelle.hpp"

namespace llvm::memoir {

class DependenceAnalysis : public llvm::noelle::DependenceAnalysis {
public:
  DependenceAnalysis(const std::string &name);

  bool canThereBeAMemoryDataDependence(Instruction *fromInst,
                                       Instruction *toInst) override;

  bool canThereBeAMemoryDataDependence(Instruction *fromInst,
                                       Instruction *toInst,
                                       Function &function) override;

  bool canThereBeAMemoryDataDependence(Instruction *fromInst,
                                       Instruction *toInst,
                                       LoopStructure &loop) override;

  MemoryDataDependenceStrength isThereThisMemoryDataDependenceType(
      DataDependenceType t,
      Instruction *fromInst,
      Instruction *toInst) override;

  MemoryDataDependenceStrength isThereThisMemoryDataDependenceType(
      DataDependenceType t,
      Instruction *fromInst,
      Instruction *toInst,
      Function &function) override;

  bool canThisDependenceBeLoopCarried(DGEdge<Value, Value> *dep,
                                      LoopStructure &loop) override;
};

} // namespace llvm::memoir

#endif // MEMOIR_DEPENDENCEANALYSIS_H
