#include "ObjectLowering.hpp"
#include "types.hpp"

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
        callee->getName().str();
        errs() << I << "\n\n";

        if (!callee) {
          // This is an indirect call, ignore for now
          continue;
        }

        auto n = callee->getName().str();

        errs() << "name is " << n << "\n";


        if (isObjectIRCall(n) && FunctionNamesToObjectIR[n] == BUILD_OBJECT) {
          errs() << "Found ObjectIR for build object!\n";
          errs() << "  " << I << "\n\n";
          
          this->buildObjects.insert(callInst);
        }
      }
    }
  }
}


void ObjectLowering::transform() {
  // Transform the program
}

ObjectWrapper *ObjectLowering::parseObjectWrapperInstruction(CallInst *i) {
    return nullptr;
}

object_lowering::Type *ObjectLowering::parseType(Instruction *ins) {
    // dispatch based on the types
    return nullptr;
}

object_lowering::Type *ObjectLowering::parseTypeCallInst(CallInst *ins) {
    return nullptr;
}

object_lowering::Type *ObjectLowering::parseTypeStoreInst(StoreInst *ins) {
    return nullptr;
}

object_lowering::Type *ObjectLowering::parseTypeLoadInst(LoadInst *ins) {
    return nullptr;
}

object_lowering::Type *ObjectLowering::parseTypeAllocaInst(AllocaInst *ins) {
    return nullptr;
}


