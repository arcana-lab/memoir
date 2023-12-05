#include "memoir/analysis/DependenceAnalysis.hpp"

namespace llvm::memoir {

static CallInst *isMemOIRCall(Instruction *i) {
  CallInst *call = dyn_cast<CallInst>(i);
  if (!call)
    return nullptr;
  if (!FunctionNames::is_memoir_call(*call))
    return nullptr;
  return call;
}

struct DependenceSummary {
  MemoryDataDependenceStrength RAW;
  MemoryDataDependenceStrength WAR;
  MemoryDataDependenceStrength WAW;

  bool exists() {
    return !(RAW == CANNOT_EXIST && WAR == CANNOT_EXIST && WAW == CANNOT_EXIST);
  }

  MemoryDataDependenceStrength query(DataDependenceType t) {
    switch (t) {
      case DG_DATA_RAW:
        return RAW;
        break;
      case DG_DATA_WAR:
        return WAR;
        break;
      case DG_DATA_WAW:
        return WAW;
        break;
      case DG_DATA_NONE:
        return MAY_EXIST;
        break;
    }
  }
};

static DependenceSummary checkDataDependenceViaDuChain(Instruction *fromInst,
                                                       Instruction *toInst) {
  // This function is only invoked when at least one of either `fromInst`
  // or `toInst` is a call to MemOIR function.
  //
  // Because MemOIR collections are in SSA:
  // * every MemOIR collection and its symbol must be immutable;
  // * an arbitrary LLVM pointer cannot alias with any MemOIR collection;
  //
  // Because of usePHI:
  // * no RAR dependency (false dependency) will be misclassified as RAW;
  auto N = toInst->getNumOperands();
  for (auto i = 0u; i < N; i++) {
    if (fromInst == toInst->getOperand(i)) {
      return DependenceSummary{
        .RAW = MUST_EXIST,
        .WAR = CANNOT_EXIST,
        .WAW = CANNOT_EXIST,
      };
    }
  }
  return DependenceSummary{
    .RAW = CANNOT_EXIST,
    .WAR = CANNOT_EXIST,
    .WAW = CANNOT_EXIST,
  };
}

static DependenceSummary checkMemOIRDataDependence(Instruction *fromInst,
                                                   Instruction *toInst) {
  CallInst *fromMemOIRCall = isMemOIRCall(fromInst);
  CallInst *toMemOIRCall = isMemOIRCall(toInst);

  // MemOIR Inst --> MemOIR Inst
  if (fromMemOIRCall && toMemOIRCall) {
    return checkDataDependenceViaDuChain(fromInst, toInst);
  }

  // MemOIR Inst --> General Inst
  else if (fromMemOIRCall && !toMemOIRCall) {
    return checkDataDependenceViaDuChain(fromInst, toInst);
  }

  // General Inst --> MemOIR Inst
  else if (!fromMemOIRCall && toMemOIRCall) {
    return checkDataDependenceViaDuChain(fromInst, toInst);
  }

  // General Inst --> General Inst
  else {
    return DependenceSummary{
      .RAW = MAY_EXIST,
      .WAR = MAY_EXIST,
      .WAW = MAY_EXIST,
    };
  }
}

DependenceAnalysis::DependenceAnalysis(const std::string &name)
  : arcana::noelle::DependenceAnalysis(name) {}

bool DependenceAnalysis::canThereBeAMemoryDataDependence(Instruction *fromInst,
                                                         Instruction *toInst) {
  return checkMemOIRDataDependence(fromInst, toInst).exists();
}

bool DependenceAnalysis::canThereBeAMemoryDataDependence(Instruction *fromInst,
                                                         Instruction *toInst,
                                                         Function &function) {
  return checkMemOIRDataDependence(fromInst, toInst).exists();
}

MemoryDataDependenceStrength DependenceAnalysis::
    isThereThisMemoryDataDependenceType(DataDependenceType t,
                                        Instruction *fromInst,
                                        Instruction *toInst) {
  return checkMemOIRDataDependence(fromInst, toInst).query(t);
}

MemoryDataDependenceStrength DependenceAnalysis::
    isThereThisMemoryDataDependenceType(DataDependenceType t,
                                        Instruction *fromInst,
                                        Instruction *toInst,
                                        Function &function) {
  return checkMemOIRDataDependence(fromInst, toInst).query(t);
}

} // namespace llvm::memoir
