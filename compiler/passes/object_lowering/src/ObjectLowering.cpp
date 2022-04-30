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
    if (inst_to_a_type.find(ins) != inst_to_a_type.end()) {
        return inst_to_a_type[ins];
    }    
    
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

    AnalysisType* a_type;

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
            auto tmp = new ObjectType();
            tmp->fields = typeVec;
            a_type = tmp;
            break;
        }
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
            a_type = new object_lowering::IntegerType(64, false); break;
        case UINT32_TYPE:
            a_type = new object_lowering::IntegerType(32, true); break;
        case UINT16_TYPE:
            a_type = new object_lowering::IntegerType(16, true); break;
        case UINT8_TYPE:
            a_type = new object_lowering::IntegerType(8, true); break;
        case INT64_TYPE:
            a_type = new object_lowering::IntegerType(64, false); break;
        case INT32_TYPE:
            a_type = new object_lowering::IntegerType(32, false); break;
        case INT16_TYPE:
            a_type = new object_lowering::IntegerType(16, false); break;
        case INT8_TYPE:
            a_type = new object_lowering::IntegerType(8, false); break;
        case FLOAT_TYPE:
            a_type = new object_lowering::FloatType(); break;
        case DOUBLE_TYPE:
            a_type = new object_lowering::DoubleType(); break;
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
    inst_to_a_type[ins] = a_type;
    return a_type;
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
        //visitedPhiNodesGlobal.insert(phiInst);
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

Value* ObjectLowering::CreateGEPFromFieldWrapper(FieldWrapper* fieldWrapper, IRBuilder<>& builder)
{
    auto int32Ty = llvm::Type::getInt32Ty(M.getContext());
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
    return gep;
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
                phiNodesToPopulate.insert(phi);
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
                auto funcType = llvm::FunctionType::get(builder.getInt8PtrTy(),
                                                        ArrayRef<Type *>({ builder.getInt64Ty() }), false);
                auto mallocf = M.getOrInsertFunction("malloc", funcType);
                auto newMallocCall = builder.CreateCall(mallocf ,arguments);

                auto bc_inst = builder.CreateBitCast(newMallocCall, PointerType::getUnqual(llvmType));

                replacementMapping[callIns] = bc_inst;
            }

            //
                //        return M.getOrInsertFunction(name, funcType);
            else if(calleeName == "writeUInt64" )//todo: improve this logic
            {
                auto fieldWrapper = readWriteFieldMap[callIns];
                auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder);
                auto storeInst = builder.CreateStore(callIns->getArgOperand(1),gep);
                replacementMapping[callIns] = storeInst;
                errs() << "out of the write gep is born" << *gep <<"\n";
                errs() << "out of the gep a store is born" << *storeInst <<"\n";
            }
            else if(calleeName == "readUInt64"){
                auto fieldWrapper = readWriteFieldMap[callIns];
                auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder);
                auto int64Ty = llvm::Type::getInt64Ty(M.getContext());
                auto loadInst = builder.CreateLoad(int64Ty,gep, "loadfrominst64");
                replacementMapping[callIns] = loadInst;
                ins.replaceAllUsesWith(loadInst);
                errs() << "out of the write gep is born" << *gep <<"\n";
                errs() << "from the readuint64 we have a load" << *loadInst <<"\n";
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
    bool debug = false;

    for (auto f : functionsToProcess)
    {
        replacementMapping.clear();
        phiNodesToPopulate.clear();
        DominatorTree &DT = mp->getAnalysis<DominatorTreeWrapperPass>(*f).getDomTree();
        auto &entry = f->getEntryBlock();
        BasicBlockTransformer(DT,&entry);

        // repopulate incoming values of phi nodes
        for (auto old_phi : phiNodesToPopulate) {
            if (replacementMapping.find(old_phi) == replacementMapping.end()) {
                if (debug) errs() << "obj_lowering transform: no new phi found for " << *old_phi << "\n";
                assert(false);
            }
            auto new_phi = dyn_cast<PHINode>(replacementMapping[old_phi]);
            assert(new_phi);
            if (debug) errs() << "will replace `" << *old_phi << "` with: `" << *new_phi << "\n";
            for (size_t i = 0; i < old_phi->getNumIncomingValues(); i++) {
                auto old_val = old_phi->getIncomingValue(i);
                if (replacementMapping.find(old_val) == replacementMapping.end()) {
                    if (debug) errs() << "obj_lowering transform: no new inst found for " << *old_val << "\n";
                    assert(false);
                }
                auto new_val = replacementMapping[old_val];
                if (debug) errs() << "Replacing operand" << i << ": " << *old_val << " with: " << *new_val << "\n";
                new_phi->addIncoming(new_val, old_phi->getIncomingBlock(i));
            }
            if (debug) errs() << "finished populating " << *new_phi << "\n";
        }
    }

    // find all instructions to delete 
    std::set<Value*> toDelete;
    for (auto p : replacementMapping) {
        toDelete.insert(p.first);
        
    }
    for (auto p : replacementMapping) {
        findInstsToDelete(p.first, toDelete);
        
    }

    errs() << "going to delete this stuff\n";
    for (auto v : toDelete) {
        errs() << *v << "\n";
        if (auto i = dyn_cast<Instruction>(v)) { 
            i->replaceAllUsesWith(UndefValue::get(i->getType()));       
            i->eraseFromParent();
        }
    }
    
}

void ObjectLowering::findInstsToDelete(Value* i, std::set<Value*> &toDelete) {
    for (auto u : i->users()) {
        if (toDelete.find(u) != toDelete.end()) continue;
        toDelete.insert(u);
        findInstsToDelete(u, toDelete);
    }
}