#include "ObjectLowering.hpp"
#include "types.hpp"
#include <functional>

using namespace object_lowering;

ObjectLowering::ObjectLowering(Module &M, Noelle *noelle, ModulePass *mp)
  : M(M),
    noelle(noelle),
    mp(mp){
  // Do initialization.
}

void ObjectLowering::analyze() {
  // Analyze the program



  errs() << "Running ObjectLowering. Analysis\n";

  for (auto &F : M) {

    if (F.getName().str() != "main") continue; // TODO: don't skip other functions
    functionsToProcess.insert(&F);
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
                case BUILD_OBJECT:
                    this->buildObjects.insert(callInst);
                    llvmObjectType = callInst->getType();
                    continue;
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
        std::set<PHINode*> visited;
        auto objT = parseObjectWrapperInstruction(ins,visited);
        buildObjMap[ins] = objT;
        errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }
    errs() << "READS\n\n";
    for(auto ins : this->reads) {
        errs() << "Parsing: " << *ins << "\n\n";
        FieldWrapper* fw;
        std::set<PHINode*> visited;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci)
        {
            fw = parseFieldWrapperIns(ci,visited);
        };
        parseType(ins->getArgOperand(0), call_back,visited);
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
        std::set<PHINode*> visited;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci)
        {
            fw = parseFieldWrapperIns(ci,visited);
        };
        parseType(ins->getArgOperand(0), call_back,visited);
        errs() << "Instruction " << *ins << "\n\n has a field wrapper where ";
        errs() <<"The base pointer is " << *(fw->baseObjPtr) << "\n";
        errs() << "The field index is" << fw->fieldIndex << "\n";
        errs() << "The type is " << fw->objectType->toString() << "\n\n\n";
        readWriteFieldMap[ins] = fw;
        //auto objT = parseObjectWrapperInstruction(ins);
        //errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }

  }
}

object_lowering::AnalysisType* ObjectLowering::parseTypeCallInst(CallInst *ins, std::set<PHINode*> &visited) {
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
//            auto firstArgVal = firstArg->get();
//            int64_t numTypeInt = dyn_cast_or_null<ConstantInt>(firstArgVal)->getSExtValue();
            for(auto arg = firstArg + 1; arg != ins->arg_end(); ++arg)
            {
                auto ins = arg->get();
                object_lowering::AnalysisType* type;
                std::set<PHINode*> visited;
                std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                    type = parseTypeCallInst(ci,visited);
                };
                parseType(dyn_cast_or_null<Instruction>(ins), call_back,visited);
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

ObjectWrapper *ObjectLowering::parseObjectWrapperChain(Value*i , std::set<PHINode*> &visited)
{
    ObjectWrapper* objw;
    std::function<void(CallInst*)> call_back = [&](CallInst* ci)
    {
//        errs() << "Field Wrapper found function " << *ci << "\n";
        objw = parseObjectWrapperInstruction(ci,visited);
    };
    parseType(i, call_back,visited);
    return objw;
}

ObjectWrapper *ObjectLowering::parseObjectWrapperInstruction(CallInst *i, std::set<PHINode*> &visited) {
    if(buildObjMap.find(i)!=buildObjMap.end())
    {
//        errs() <<" This function has been mapped earlier through buildObjMap\n";
        return  buildObjMap[i];
    }

    auto arg = i->arg_begin()->get();
    AnalysisType* type;
    std::function<void(CallInst*)> callback = [&](CallInst* ci)
    {
        type = parseTypeCallInst(ci,visited);
    };
    parseType(dyn_cast_or_null<Instruction>(arg),callback,visited);
//    errs() << "Obtained ObjectWrapper AnalysisType for " << *i <<"\n";

    if(type->getCode() != ObjectTy)
    {
        errs() << "It's not an object";
        assert(false);
    }
    auto* objt = (ObjectType*) type;
    return new ObjectWrapper(objt);
}




void ObjectLowering::parseType(Value *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*>& visited) {
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
        parseTypeStoreInst(storeIns,callback,visited);
        return;
    }
    else if(auto loadIns = dyn_cast_or_null<LoadInst>(ins))
    {
        parseTypeLoadInst(loadIns,callback,visited);
        return;
    }
    else if (auto allocaIns = dyn_cast_or_null<AllocaInst>(ins))
    {
        parseTypeAllocaInst(allocaIns,callback,visited);
        return;
    } else if (auto gv = dyn_cast_or_null<GlobalValue>(ins)) {
        parseTypeGlobalValue(gv,callback,visited);
        return;
    } else if (auto phiInst = dyn_cast_or_null<PHINode>(ins)) {
        /*
         *  loop through all incoming values
         *      if the value has been visited
         *          do nothing
         *      if not:
         *          call call back
         */

//        errs() << "parse type phi " << *phiInst << "\n";
        if(visited.find(phiInst) != visited.end())
        {
            return;
        }
        visited.insert(phiInst);
        visitedPhiNodesGlobal.insert(phiInst);
        for(auto& val: phiInst->incoming_values())
        {
            parseType(val.get(), callback,visited);
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



void ObjectLowering::parseTypeStoreInst(StoreInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    auto valOp = ins->getValueOperand();
    parseType(dyn_cast_or_null<Instruction>(valOp), callback,visited);
}

void ObjectLowering::parseTypeLoadInst(LoadInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    auto ptrOp = ins->getPointerOperand();

    /*if (auto gv = dyn_cast<GlobalValue>(ptrOp)) {
        errs() << *gv << " is a global value\n";
        for(auto u : gv->users())
        {
            errs() << "\t" << *u << "\n";
        }
    }
    assert(false);*/

    parseType(ptrOp,callback,visited);
}

void ObjectLowering::parseTypeAllocaInst(AllocaInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    for(auto u: ins->users())
    {
        if(auto i = dyn_cast_or_null<StoreInst>(u))
        {
            return parseType(i,callback,visited);
        }
    }
    errs() << "Didn't find any store instruction uses for the instruction" <<*ins;
    assert(false);
}

void ObjectLowering::parseTypeGlobalValue(GlobalValue *gv, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    for(auto u: gv->users())
    {
        if(auto i = dyn_cast_or_null<StoreInst>(u))
        {
            return parseType(i,callback,visited);
        }
    }
    errs() << "Didn't find any store instruction uses for the gv" << *gv;
    assert(false);
}



FieldWrapper* ObjectLowering::parseFieldWrapperIns(CallInst* i, std::set<PHINode*> &visited)
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
    errs() << "The Field instruction is  " << *i <<"\n";
    int64_t fieldIndex = CI->getSExtValue();
    auto objw = parseObjectWrapperChain(firstarg, visited);
//    errs() << "Obtained Field Wrapper AnalysisType for " << *i <<"\n";
    auto fieldwrapper = new FieldWrapper();
    fieldwrapper->baseObjPtr = firstarg;
    fieldwrapper->fieldIndex = fieldIndex; // NOLINT(cppcoreguidelines-narrowing-conversions)
    fieldwrapper->objectType = objw->innerType;
    return fieldwrapper;
}



// ========================================================================

void ObjectLowering::BasicBlockTransformer(DominatorTree &DT, BasicBlock *bb)
{
    errs() << "Transforming Basic Block  " <<*bb << "\n\n";
    auto int64Ty = llvm::Type::getInt64Ty(M.getContext());

    auto int32Ty = llvm::Type::getInt32Ty(M.getContext());
    for(auto &ins: *bb)
    {
        errs() << "encountering  instruction " << ins <<"\n";
        IRBuilder<> builder(&ins);
        if(auto phi = dyn_cast<PHINode>(&ins))
        {
            errs()<< "The phi has type " << *phi->getType() <<"\n";
            errs() << "our type is " << *llvmObjectType << "\n";
            if(phi->getType() == llvmObjectType)
            {
                errs() << "those two types as equal" <<"\n";
                std::set<PHINode*> visited;
                ObjectWrapper* objw = parseObjectWrapperChain(phi,visited);
                auto llvmType = objw->innerType->getLLVMRepresentation(M);
                auto llvmPtrType = PointerType::getUnqual(llvmType);
                auto newPhi = builder.CreatePHI(llvmPtrType, phi->getNumIncomingValues() );
                errs() << "out of the old phi a new one is born" << *newPhi <<"\n";
                replacementMapping[phi] = newPhi;
            }
        }
        else if(auto callIns = dyn_cast<CallInst>(&ins))
        {
            auto callee = callIns->getCalledFunction();
            if(callee == nullptr)
            {
                continue;
            }
            auto calleeName = callee->getName().str();
            if(calleeName == "buildObject")
            {
                auto llvmType = buildObjMap[callIns]->innerType->getLLVMRepresentation(M);
                auto llvmTypeSize = llvm::ConstantInt::get(int64Ty, M.getDataLayout().getTypeAllocSize(llvmType));
                std::vector<Value *> arguments{llvmTypeSize};
                auto mallocf = M.getFunction("malloc");
                auto newMallocCall = builder.CreateCall(mallocf ,arguments);
                replacementMapping[callIns] = newMallocCall;
            }
            else if(calleeName == "writeUInt64")//todo: improve this logic
            {
                auto fieldWrapper = readWriteFieldMap[callIns];
                errs() << "Instruction " << *callIns << "\n\n has a field wrapper where ";
                errs() <<"The base pointer is " << *(fieldWrapper->baseObjPtr) << "\n";
                errs() << "The field index is" << fieldWrapper->fieldIndex << "\n";
                errs() << "The type is " << fieldWrapper->objectType->toString() << "\n\n\n";

                auto llvmType = fieldWrapper->objectType->getLLVMRepresentation(M);
                std::vector<Value*> indices = {llvm::ConstantInt::get(int32Ty, 0),
                                               llvm::ConstantInt::get(int32Ty,fieldWrapper->fieldIndex )};
                if(replacementMapping.find(fieldWrapper->baseObjPtr) ==replacementMapping.end())
                {
                    errs() << "unable to find the base pointer " << *fieldWrapper->baseObjPtr <<"\n";
                    assert(false);
                }
                auto llvmPtrType = PointerType::getUnqual(llvmType);
                auto gep = builder.CreateGEP(replacementMapping[fieldWrapper->baseObjPtr],indices);
                auto storeInst = builder.CreateStore(callIns->getArgOperand(1),gep);
                replacementMapping[callIns] = storeInst;
                errs() << "out of the write a store is born" << *storeInst <<"\n";
            }
        }
    }

    auto node = DT.getNode(bb);
    for(auto child: node->getChildren())
    {
        auto dominated = child->getBlock();
        BasicBlockTransformer(DT, dominated);
    }
}

void ObjectLowering::transform() {
    for (auto f : functionsToProcess)
    {
        DominatorTree &DT = mp->getAnalysis<DominatorTreeWrapperPass>(*f).getDomTree();
        auto &entry = f->getEntryBlock();
        BasicBlockTransformer(DT,&entry);
    }


//    auto &context = M.getContext();
//    auto int64Ty = llvm::Type::getInt64Ty(context);
//
//    for(auto ins : this->buildObjects) {
//        std::set<PHINode*> visited;
//        auto objT = parseObjectWrapperInstruction(ins,visited);
//        auto llvmType = objT->innerType->getLLVMRepresentation(M);
//        errs() << *llvmType <<"\n";
//        auto llvmTypeSize = llvm::ConstantInt::get(int64Ty, M.getDataLayout().getTypeAllocSize(llvmType));
//        IRBuilder<> builder(ins);
//        std::vector<Value *> arguments{llvmTypeSize};
//        auto smallf = M.getFunction("malloc");
//        builder.CreateCall(smallf ,arguments);
//    }


}
