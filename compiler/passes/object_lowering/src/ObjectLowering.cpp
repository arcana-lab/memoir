#include "ObjectLowering.hpp"

using namespace object_lowering;

ObjectLowering::ObjectLowering(Module &M, Noelle *noelle)
  : M(M),
    noelle(noelle) {
  // Do initialization.
}

void ObjectLowering::analyze() {
  // Analyze the program

  errs() << "Running ObjectLowering. Analysis\n";

  for (auto &F : M) {

    if (F.getName().str() != "main") continue;

    errs() << "Found function main\n";

    for (auto &I : instructions(F)) {

      if (auto callInst = dyn_cast<CallInst>(&I)) {
        auto callee = callInst->getCalledFunction();

        errs() << I << "\n\n";

        if (!callee) {
          // This is an indirect call, ignore for now
          continue;
        }

        auto n = callee->getName();

        errs() << "name is " << n << "\n";


        if (isObjectIRCall(n)) {
          errs() << "Found ObjectIR Call!\n";
          errs() << "  " << I << "\n\n";
          
          this->callsToObjectIR.insert(callInst);
        }
      }
    }
  }
}

void ObjectLowering::transform() {
  // Transform the program
}
