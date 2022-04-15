#include "ObjectLowering.hpp"

using namespace object_lowering;

ObjectLowering::ObjectLowering(Module &M, Noelle *noelle)
  : M(M),
    noelle(noelle) {
  // Do initialization.
}

void ObjectLowering::analyze() {
  // Analyze the program

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
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
}

void ObjectLowering::transform() {
  // Transform the program
}
