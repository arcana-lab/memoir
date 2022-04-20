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

        if (callee == nullptr) {
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
    // dispatch on the dynamic type of ins
    if(auto callins = dyn_cast<CallInst>(ins))
    {
        return parseTypeCallInst(callins);
    }
    else if (auto storeIns = dyn_cast<StoreInst>(ins))
    {
        return parseTypeStoreInst(storeIns);
    }
    else if(auto loadIns = dyn_cast<LoadInst>(ins))
    {
        return parseTypeLoadInst(loadIns);
    }
    else if (auto allocaIns = dyn_cast<AllocaInst>(ins))
    {
        return parseTypeAllocaInst(allocaIns);
    }
    // we can't handle this so we just like low key give up
    errs() << "Unrecognized Instruction" << *ins <<"\n";
    assert(false);
    return nullptr;
}

object_lowering::Type *ObjectLowering::parseTypeCallInst(CallInst *ins) {
    // check to see what sort of call instruction this is dispatch on the name of the function
    auto callee = ins->getCalledFunction();
    if (!callee) {
        errs() << "Unrecognized indirect call" << *ins << "\n";
        assert(false);
    }
    auto n = callee->getName().str();
    if (!isObjectIRCall(n)) {
        errs() << "Unrecognized function call " << *ins << "\n";
        assert(false);
    }
    switch (FunctionNamesToObjectIR[n])
    {
        case OBJECT_TYPE:
            break;
        case ARRAY_TYPE:
            break;
        case UNION_TYPE:
            break;
        case INTEGER_TYPE:
            break;
        case UINT64_TYPE:
            break;
        case UINT32_TYPE:
            break;
        case UINT16_TYPE:
            break;
        case UINT8_TYPE:
            break;
        case INT64_TYPE:
            break;
        case INT32_TYPE:
            break;
        case INT16_TYPE:
            break;
        case INT8_TYPE:
            break;
        case FLOAT_TYPE:
            break;
        case DOUBLE_TYPE:
            break;
        case BUILD_OBJECT:
            break;
        case BUILD_ARRAY:
            break;
        case BUILD_UNION:
            break;
        default:
            errs() <<"the switch should cover everything this is wrong\n";
            assert(false);
            break;
    }
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


