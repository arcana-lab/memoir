#include "StructOfArrays.hpp"

StructOfArrays::StructOfArrays(Module &M, Noelle *noelle)
  : M(M),
    noelle(noelle) {
  // Do initialization.
}

void StructOfArrays::analyze() {
  // Analyze the program

  for (auto &F : M) {
    for (auto &I : F) {
      if (auto callInst = dyn_cast<CallInst>(&I)) {
        auto callee = callInst->getCalledFunction();

        if (!callee) {
          // This is an indirect call, ignore for now
          continue;
        }

        if (isObjectIRCall(callee->getName())) {
          errs() << "Found ObjectIR Call!\n";
          errs() << "  " << I << "\n\n";

          this->callsToObjectIR.insert(callInst);
        }
      }
    }
  }
}

void StructOfArrays::transform() {
  // Transform the program
}
