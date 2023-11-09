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
  CallInst *fromCall = dyn_cast<CallInst>(fromInst);
  CallInst *toCall = dyn_cast<CallInst>(toInst);
  if (fromCall == nullptr || toCall == nullptr)
    return true;

  MemOIR_Func fromFunc = FunctionNames::get_memoir_enum(*fromCall);
  MemOIR_Func toFunc = FunctionNames::get_memoir_enum(*toCall);
  if (fromFunc == MemOIR_Func::NONE || toFunc == MemOIR_Func::NONE)
    return true; // TODO

  // TODO: let `equal` to account for phi and select
  // TODO: handle instructions other than ACCESS
  //
  // TYPE
  //   ...
  //
  // ALLOC
  //   ...
  //
  // ACCESS
  //   READ
  //     STRUCT_READ
  //     INDEX_READ
  //     ASSOC_READ
  //   WRITE
  //     STRUCT_WRITE
  //     INDEX_WRITE
  //     ASSOC_WRITE
  //   GET
  //     STRUCT_GET
  //     INDEX_GET
  //     ASSOC_GET
  //
  // SEQ_INSERT
  // SEQ_REMOVE
  // SEQ_APPEND
  // SEQ_SWAP
  // SEQ_SPLIT

  if ((FunctionNames::is_read(fromFunc) || FunctionNames::is_write(fromFunc))
      && (FunctionNames::is_read(toFunc) || FunctionNames::is_write(toFunc))) {
    // TODO: def phi will soon be gone
    return false;
  }

  if (FunctionNames::is_get(fromFunc) && FunctionNames::is_get(toFunc))
    return false;

  if (toFunc == MemOIR_Func::DEF_PHI) {
    if (FunctionNames::is_write(fromFunc)) {
      Value *written = fromCall->getArgOperand(1);
      Value *def = toCall->getArgOperand(0);
      if (written == def)
        return true; // TODO this is a must
    }
    return false;
  }

  if (toFunc == MemOIR_Func::USE_PHI) {
    if (FunctionNames::is_read(fromFunc)) {
      Value *read = fromCall->getArgOperand(0);
      Value *use = toCall->getArgOperand(0);
      if (read == use)
        return true; // TODO this is a must
    }
    return false;
  }

  if (fromFunc == MemOIR_Func::DEF_PHI || fromFunc == MemOIR_Func::USE_PHI) {
    Value *access = FunctionNames::is_read(toFunc)
                        ? toCall->getArgOperand(0)
                        : FunctionNames::is_write(toFunc)
                              ? toCall->getArgOperand(1)
                              : FunctionNames::is_get(toFunc)
                                    ? toCall->getArgOperand(0)
                                    : nullptr;
    if (access) {
      if (access == fromCall) {
        return true; // TODO this is a must
      } else {
        return false;
      }
    }
  }

  return true;
}

bool DependenceAnalysis::canThereBeAMemoryDataDependence(Instruction *fromInst,
                                                         Instruction *toInst,
                                                         LoopStructure &loop) {
  println("(^o^) LOOP (^o^)");
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
