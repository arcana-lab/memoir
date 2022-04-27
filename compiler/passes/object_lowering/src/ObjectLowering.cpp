#include "ObjectLowering.hpp"
#include "types.hpp"
#include <functional>

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

    if (F.getName().str() != "main") continue; // TODO: don't skip other functions

    for (auto &I : instructions(F)) {

      if (auto callInst = dyn_cast_or_null<CallInst>(&I)) {
        auto callee = callInst->getCalledFunction();

        if (callee == nullptr) {
          // This is an indirect call, ignore for now
          continue;
        }

        auto n = callee->getName().str();

        if (isObjectIRCall(n)) {
            switch (FunctionNamesToObjectIR[n]) {
                case BUILD_OBJECT: this->buildObjects.insert(callInst); continue;
                case READ_UINT64: this->reads.insert(callInst); continue;
                case WRITE_OBJECT:
                case WRITE_UINT64: this->writes.insert(callInst); continue;
                default: continue;
            }         
        }
      }
    }

    for(auto ins : this->buildObjects) {
        //errs() << "Parsing: " << *ins << "\n\n";
        auto objT = parseObjectWrapperInstruction(ins);
        buildObjMap[ins] = objT;
        errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }
    errs() << "READS\n\n";
    for(auto ins : this->reads) {
        errs() << "Parsing: " << *ins << "\n\n";
        FieldWrapper* fw;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci)
        {
            fw = parseFieldWrapperIns(ci);
        };
        parseType(ins->getArgOperand(0), call_back);
        errs() << "Instruction " << *ins << "\n\n has a field wrapper where ";
        errs() <<"The base pointer is " << *(fw->baseObjPtr) << "\n";
        errs() << "The field index is" << fw->fieldIndex << "\n";
        errs() << "The type is " << fw->objectType->toString() << "\n\n\n";
        readWriteFieldMap[ins] = fw;
        //auto objT = parseObjectWrapperInstruction(ins);
        //errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }
    errs() << "WRITES\n\n";
    for(auto ins : this->writes) {
        errs() << "Parsing: " << *ins << "\n\n";
        FieldWrapper* fw;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci)
        {
            fw = parseFieldWrapperIns(ci);
        };
        readWriteFieldMap[ins] = fw;
        parseType(ins->getArgOperand(0), call_back);
        errs() << "Instruction " << *ins << "\n\n has a field wrapper where ";
        errs() <<"The base pointer is " << *(fw->baseObjPtr) << "\n";
        errs() << "The field index is" << fw->fieldIndex << "\n";
        errs() << "The type is " << fw->objectType->toString() << "\n\n\n";
        //auto objT = parseObjectWrapperInstruction(ins);
        //errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }

  }
}

object_lowering::AnalysisType* ObjectLowering::parseTypeCallInst(CallInst *ins) {
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
        {std::vector<object_lowering::AnalysisType*> typeVec;
            auto firstArg = ins->arg_begin();
            auto firstArgVal = firstArg->get();
            int64_t numTypeInt = dyn_cast_or_null<ConstantInt>(firstArgVal)->getSExtValue();
            for(auto arg = firstArg + 1; arg != ins->arg_end(); ++arg)
            {
                auto ins = arg->get();
                object_lowering::AnalysisType* type;
                std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                    type = parseTypeCallInst(ci);
                };
                parseType(dyn_cast_or_null<Instruction>(ins), call_back);
                typeVec.push_back(type);
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
            return new object_lowering::IntegerType(64, false);
        case UINT32_TYPE:
            return new object_lowering::IntegerType(32, true);
        case UINT16_TYPE:
            return new object_lowering::IntegerType(16, true);
        case UINT8_TYPE:
            return new object_lowering::IntegerType(8, true);
        case INT64_TYPE:
            return new object_lowering::IntegerType(64, false);
        case INT32_TYPE:
            return new object_lowering::IntegerType(32, false);
        case INT16_TYPE:
            return new object_lowering::IntegerType(16, false);
        case INT8_TYPE:
            return new object_lowering::IntegerType(8, false);
        case FLOAT_TYPE:
            return new object_lowering::FloatType();
        case DOUBLE_TYPE:
            return new object_lowering::DoubleType();
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

ObjectWrapper *ObjectLowering::parseObjectWrapperInstruction(CallInst *i) {
    auto arg = i->arg_begin()->get();
    AnalysisType* type;
    std::function<void(CallInst*)> callback = [&](CallInst* ci)
    {
        type = parseTypeCallInst(ci);
    };
    parseType(dyn_cast_or_null<Instruction>(arg),callback );
//    errs() << "Obtained ObjectWrapper AnalysisType for " << *i <<"\n";

    if(type->getCode() != ObjectTy)
    {
        errs() << "It's not an object";
        assert(false);
    }
    auto* objt = (ObjectType*) type;
    return new ObjectWrapper(objt);
}




void ObjectLowering::parseType(Value *ins, std::function<void(CallInst*)> callback) {
    // dispatch on the dynamic type of ins
//    errs()<<*ins << "is being called by parseType\n";

    if (auto callins = dyn_cast_or_null<CallInst>(ins))
    {
        callback(callins);
        return;
        //return parseTypeCallInst(callins);
    }
    else if (auto storeIns = dyn_cast_or_null<StoreInst>(ins))
    {
        parseTypeStoreInst(storeIns,callback);
        return;
    }
    else if(auto loadIns = dyn_cast_or_null<LoadInst>(ins))
    {
        parseTypeLoadInst(loadIns,callback);
        return;
    }
    else if (auto allocaIns = dyn_cast_or_null<AllocaInst>(ins))
    {
        parseTypeAllocaInst(allocaIns,callback);
        return;
    } else if (auto gv = dyn_cast_or_null<GlobalValue>(ins)) {
        parseTypeGlobalValue(gv,callback);
        return;
    } else if (auto phiInst = dyn_cast_or_null<PHINode>(ins)) {
        /*
         *  loop through all incoming values
         *      if the value has been visited
         *          do nothing
         *      if not:
         *          call call back
         */

        errs() << "parse type phi " << phiInst << "\n";
        if(visitedPhiNodes.find(phiInst)!= visitedPhiNodes.end())
        {
            return;
        }
        visitedPhiNodes.insert(phiInst);
        for(auto& val: phiInst->incoming_values())
        {
            parseType(val.get(), callback);
        }
        return;
    } else if (!ins) {
        errs() << "i think this is a nullptr\n";
        assert(false); 
    }
    // we can't handle this so we just like low key give up
    errs() << "Unrecognized Instruction" << *ins <<"\n";
    assert(false);
}



void ObjectLowering::parseTypeStoreInst(StoreInst *ins, std::function<void(CallInst*)> callback) {
    auto valOp = ins->getValueOperand();
    parseType(dyn_cast_or_null<Instruction>(valOp), callback);
}

void ObjectLowering::parseTypeLoadInst(LoadInst *ins, std::function<void(CallInst*)> callback) {
    auto ptrOp = ins->getPointerOperand();

    /*if (auto gv = dyn_cast<GlobalValue>(ptrOp)) {
        errs() << *gv << " is a global value\n";
        for(auto u : gv->users())
        {
            errs() << "\t" << *u << "\n";
        }
    }
    assert(false);*/

    parseType(ptrOp,callback);
}

void ObjectLowering::parseTypeAllocaInst(AllocaInst *ins, std::function<void(CallInst*)> callback) {
    for(auto u: ins->users())
    {
        if(auto i = dyn_cast_or_null<StoreInst>(u))
        {
            return parseType(i,callback);
        }
    }
    errs() << "Didn't find any store instruction uses for the instruction" <<*ins;
    assert(false);
}

void ObjectLowering::parseTypeGlobalValue(GlobalValue *gv, std::function<void(CallInst*)> callback) {
    for(auto u: gv->users())
    {
        if(auto i = dyn_cast_or_null<StoreInst>(u))
        {
            return parseType(i,callback);
        }
    }
    errs() << "Didn't find any store instruction uses for the gv" << *gv;
    assert(false);
}



FieldWrapper* ObjectLowering::parseFieldWrapperIns(CallInst* i)
{
    auto callee = i->getCalledFunction();
    if (!callee) {
        errs() << "Unrecognized indirect call" << *i << "\n";
        assert(false);
    }
    auto n = callee->getName().str();
    if(n != "getObjectField")
    {
        errs() << "Def use chain gave us the wrong function?" << *i;
        assert(false);
    }
    auto firstarg = i->getArgOperand(0);
    auto secondarg =  i->getArgOperand(1);
    auto CI = dyn_cast_or_null<ConstantInt>(secondarg);
    assert(CI);
    int64_t fieldIndex = CI->getSExtValue();
    ObjectWrapper* objw;
    std::function<void(CallInst*)> call_back = [&](CallInst* ci)
    {
        if(buildObjMap.find(ci)!=buildObjMap.end())
        {
            objw = buildObjMap[ci];
        }
        else {
            objw = parseObjectWrapperInstruction(ci);
        }
    };
    parseType(firstarg, call_back);
//    errs() << "Obtained Field Wrapper AnalysisType for " << *i <<"\n";
    auto fieldwrapper = new FieldWrapper();
    fieldwrapper->baseObjPtr = firstarg;
    fieldwrapper->fieldIndex = fieldIndex; // NOLINT(cppcoreguidelines-narrowing-conversions)
    fieldwrapper->objectType = objw->innerType;
    return fieldwrapper;
}


// ========================================================================

void ObjectLowering::transform() {
    auto* TD = new DataLayout(&M);
    auto &context = M.getContext();
    auto int64Ty = llvm::Type::getInt64Ty(context);
//    std::vector<llvm::AnalysisType *> types{int64Ty};
//    auto llvm::StructType::create(M.getContext(), types, "my_struct", false);
//
//    auto int64size =

    for(auto ins : this->buildObjects) {
        auto objT = parseObjectWrapperInstruction(ins);
        auto llvmType = objT->innerType->getLLVMRepresentation(M);
        errs() << *llvmType <<"\n";
        auto llvmTypeSize = llvm::ConstantInt::get(int64Ty, M.getDataLayout().getTypeAllocSize(llvmType));
        IRBuilder<> builder(ins);
        std::vector<Value *> arguments{llvmTypeSize};
        auto smallf = M.getFunction("malloc");
        builder.CreateCall(smallf ,arguments);
    }









    // get module ctxt
    // fetch the first buildObject instruction
    // get its type
    // construct the corresponding ArrRef
    // create the struct type
    // create alloca inst
}
