#include "memoir/analysis/DependenceAnalysis.hpp"

namespace llvm::memoir {

DependenceAnalysis::DependenceAnalysis(const std::string &name)
  : noelle::DependenceAnalysis(name) {}

bool DependenceAnalysis::canThereBeAMemoryDataDependence(Instruction *fromInst,
                                                         Instruction *toInst) {
  return true;
}

bool DependenceAnalysis::canThereBeAMemoryDataDependence(Instruction *fromInst,
                                                         Instruction *toInst,
                                                         Function &function) {
  return true;
}

bool DependenceAnalysis::canThereBeAMemoryDataDependence(Instruction *fromInst,
                                                         Instruction *toInst,
                                                         LoopStructure &loop) {
  CallInst *fromCall = dyn_cast<CallInst>(fromInst);
  CallInst *toCall = dyn_cast<CallInst>(toInst);
  if (fromCall == nullptr || toCall == nullptr)
    return true;

  MemOIR_Func fromFunc = FunctionNames::get_memoir_enum(*fromCall);
  MemOIR_Func toFunc = FunctionNames::get_memoir_enum(*toCall);
  if (fromFunc == MemOIR_Func::NONE || toFunc == MemOIR_Func::NONE)
    return true;

  if (FunctionNames::is_get(fromFunc) && FunctionNames::is_get(fromFunc))
    return false;

  return true;
}

MemoryDataDependenceStrength DependenceAnalysis::
    isThereThisMemoryDataDependenceType(DataDependenceType t,
                                        Instruction *fromInst,
                                        Instruction *toInst) {
  return MAY_EXIST;
}

MemoryDataDependenceStrength DependenceAnalysis::
    isThereThisMemoryDataDependenceType(DataDependenceType t,
                                        Instruction *fromInst,
                                        Instruction *toInst,
                                        Function &function) {
  return MAY_EXIST;
}

MemoryDataDependenceStrength DependenceAnalysis::
    isThereThisMemoryDataDependenceType(DataDependenceType t,
                                        Instruction *fromInst,
                                        Instruction *toInst,
                                        LoopStructure &loop) {
  return MAY_EXIST;
}

bool DependenceAnalysis::canThisDependenceBeLoopCarried(
    DGEdge<Value, Value> *dep,
    LoopStructure &loop) {
  return true;
}

} // namespace llvm::memoir
