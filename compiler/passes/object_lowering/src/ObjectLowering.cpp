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
        errs() << I << "\n\n";

        if (callee == nullptr) {
          // This is an indirect call, ignore for now
          continue;
        }

        auto n = callee->getName().str();

        errs() << "name is " << n;


        if (isObjectIRCall(n) && FunctionNamesToObjectIR[n] == BUILD_OBJECT) {
          errs() << "Found ObjectIR for build object!\n";
          errs() << "  " << I << "\n\n";
          
          this->buildObjects.insert(callInst);
        }

        errs() << "got here\n";


      }
    }

    for(auto ins : this->buildObjects)
    {
        auto objT = parseObjectWrapperInstruction(ins);
        errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }

  }
}


void ObjectLowering::transform() {
  // Transform the program
}

ObjectWrapper *ObjectLowering::parseObjectWrapperInstruction(CallInst *i) {
    auto arg = i->arg_begin()->get();
    auto type = parseType(dyn_cast<Instruction>(arg));
    if(type->getCode() != ObjectTy)
    {
        errs() << "It's not an object";
        assert(false);
    }
    auto* objt = (ObjectType*) type;
    return new ObjectWrapper(objt);
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
            {std::vector<Type*> typeVec;
                auto firstArg = ins->arg_begin();
                auto firstArgVal = firstArg->get();
                int64_t numTypeInt = dyn_cast<ConstantInt>(firstArgVal)->getSExtValue();
                for(auto arg = firstArg + 1; arg != ins->arg_end(); ++arg)
                {
                    auto ins = arg->get();
                    typeVec.push_back(parseType(dyn_cast<Instruction>(ins)));
                }
                auto objType = new ObjectType();
                objType->fields = typeVec;
                return objType;}
        case ARRAY_TYPE:
            assert(false);
            break;
        case UNION_TYPE:
            assert(false);
            break;
        case INTEGER_TYPE:
            assert(false);
//            break;
        case UINT64_TYPE:
            return new IntegerType(64, false);
        case UINT32_TYPE:
            return new IntegerType(32, true);
        case UINT16_TYPE:
            return new IntegerType(16, true);
        case UINT8_TYPE:
            return new IntegerType(8, true);;
        case INT64_TYPE:
            return new IntegerType(64, false);
        case INT32_TYPE:
            return new IntegerType(32, false);
        case INT16_TYPE:
            return new IntegerType(16, false);
        case INT8_TYPE:
            return new IntegerType(8, false);
        case FLOAT_TYPE:
            return new FloatType();
        case DOUBLE_TYPE:
            return new DoubleType();
        case BUILD_OBJECT:
            errs() << "There shouldn't be a build object in this chain \n";
            assert(false);
            break;
        case BUILD_ARRAY:
            assert(false);
            break;
        case BUILD_UNION:
            assert(false);
            break;
        default:
            errs() <<"the switch should cover everything this is wrong\n";
            assert(false);
            break;
    }
    return nullptr;
}

object_lowering::Type *ObjectLowering::parseTypeStoreInst(StoreInst *ins) {
    auto valOp = ins->getValueOperand();
    return parseType(dyn_cast<Instruction>(valOp));
}

object_lowering::Type *ObjectLowering::parseTypeLoadInst(LoadInst *ins) {
    auto ptrOp = ins->getPointerOperand();
    return parseType(dyn_cast<Instruction>(ptrOp));
}

object_lowering::Type *ObjectLowering::parseTypeAllocaInst(AllocaInst *ins) {
    for(auto u: ins->users())
    {
        if(auto i = dyn_cast<StoreInst>(u))
        {
            return parseType(i);
        }
    }
    errs() << "Didn't find any store insturction uses for the instruction" <<*ins;
    assert(false);
}


