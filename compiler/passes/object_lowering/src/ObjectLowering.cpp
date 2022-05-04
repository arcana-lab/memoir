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
  errs() << "\n\nRunning ObjectLowering::Analysis\n";

  // Hack to get the LLVM::Type* representation of ObjectIR::Type*
  auto getTypeFunc = M.getFunction("getUInt64Type");
  auto type_star = getTypeFunc->getReturnType();
  auto type_star_star = PointerType::getUnqual(type_star);

  // collect all GlobalVals which are Type*
  std::vector<GlobalValue*> typeDefs;
  for (auto &globalVar : M.getGlobalList()) {
      if (globalVar.getType() == type_star_star) {
          typeDefs.push_back(&globalVar);
      }
  }
 
   std::map<string, AnalysisType*> namedTypeMap;
   std::vector<AnalysisType*> allTypes;

  // parse the Types
  for (auto v : typeDefs) {
    errs() << "Global value: " << *v << "\n";
    object_lowering::AnalysisType* type;
    std::set<PHINode*> visited;
    std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
        type = parseTypeCallInst(ci,visited);
    };
    parseType(v, call_back,visited);

    allTypes.push_back(type);
    if(type->getCode() == ObjectTy) {
        auto objTy = (ObjectType*) type;
        if (objTy->hasName()) namedTypeMap[objTy->name] = objTy;
    }

    
  }
  // assume that 1) all named Types ARE global vals and not intermediate
  // 2) all un-named types are also global vals

  // resolve stubs
  for (auto type : allTypes) {
     if(type->getCode() == ObjectTy) {
        
        auto objTy = (ObjectType*) type;
        for (auto fld : objTy->fields) {
            if(fld->getCode() == PointerTy) {
                auto ptrTy = (APointerType*) fld;
                auto pointsTo = ptrTy->pointsTo;
                if(pointsTo->getCode() == StubTy) {
                    auto stubTy = (StubType*) pointsTo;
                    ptrTy->pointsTo = namedTypeMap[stubTy->name];
                }
            }
        }
        
    }
  }

  for (auto v : allTypes) {
      errs() << v->toString() << "\n\n";
  }

  // the types generated above are cached by parseTypeCallInst, so we could safely delete the helper maps
  // ===== code before names types were merged ===

  for (auto &F : M) {

    if (F.getName().str() != "main") continue; // TODO: don't skip other functions
    functionsToProcess.insert(&F);

    // collect all of the relevant CallInsts
    for (auto &I : instructions(F)) {

      if (auto callInst = dyn_cast_or_null<CallInst>(&I)) {
        auto callee = callInst->getCalledFunction();
        if (callee == nullptr) continue; // This is an indirect call, ignore for now
        auto n = callee->getName().str();

        if (isObjectIRCall(n)) {
            switch (FunctionNamesToObjectIR[n]) {
                case BUILD_OBJECT:
                    this->buildObjects.insert(callInst);
                    llvmObjectType = callInst->getType(); // setup the hacky llvm::ObjectType*
                    continue;
                case READ_POINTER:
                case READ_UINT64: this->reads.insert(callInst); continue;
                case WRITE_POINTER:
                case WRITE_UINT64: this->writes.insert(callInst); continue;
                default: continue;
            }         
        }
      }
    }

    // construct the ObjectWrapper* for each @buildObject call
    for(auto ins : this->buildObjects) {
        errs() << "Parsing: " << *ins << "\n\n";
        std::set<PHINode*> visited;
        auto objT = parseObjectWrapperInstruction(ins,visited);
//        buildObjMap[ins] = objT;
        errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }


    // construct the FieldWrapper* for each read, write call
    errs() << "READS\n\n";
    for(auto ins : this->reads) {
        errs() << "Parsing: " << *ins << "\n\n";
        FieldWrapper* fw;
        std::set<PHINode*> visited;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
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
} // endof analyze

object_lowering::AnalysisType* ObjectLowering::parseTypeCallInst(CallInst *ins, std::set<PHINode*> &visited) {
    // return a cached AnalysisType*
    if (analysisTypeMap.find(ins) != analysisTypeMap.end()) {
        return analysisTypeMap[ins];
    }    
    
    auto callee = ins->getCalledFunction();
    if (!callee) {
        //errs() << "Unrecognized indirect call" << *ins << "\n";
        assert(false);
    }
    auto n = callee->getName().str();
    if (!isObjectIRCall(n)) {
        //errs() << "Unrecognized function call " << *ins << "\n";
        assert(false);
    }

    AnalysisType* a_type; // the Type* of this CallInst will be reconstructed into an AnalysisType*

    switch (FunctionNamesToObjectIR[n])
    {
        case OBJECT_TYPE: {
            std::vector<object_lowering::AnalysisType*> typeVec;
            for(auto arg = ins->arg_begin() + 1; arg != ins->arg_end(); ++arg)
            {
                auto ins = arg->get();
                object_lowering::AnalysisType* type;
//                std::set<PHINode*> visited;
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
        case NAME_OBJECT_TYPE: {
            std::vector<object_lowering::AnalysisType*> typeVec;
            auto name = fetchString(ins->arg_begin()->get());
            // the same as BUILD_OBJECT, except start from the 3rd argument (skip the name & # of args)
            for(auto arg = ins->arg_begin() + 2; arg != ins->arg_end(); ++arg)
            {
                auto ins = arg->get();
                object_lowering::AnalysisType* type;
//                std::set<PHINode*> visited;
                std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                    type = parseTypeCallInst(ci,visited);
                };
                parseType(dyn_cast_or_null<Instruction>(ins), call_back,visited);
                typeVec.push_back(type);
            }
            auto tmp = new ObjectType();
            tmp->name = name;
            tmp->fields = typeVec;
            a_type = tmp;
            break;
        }
        case POINTER_TYPE: {
            auto pointsToVal = ins->getArgOperand(0);
            object_lowering::AnalysisType* type;
            std::set<PHINode*> visited;
            std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                type = parseTypeCallInst(ci,visited);
            };
            parseType(pointsToVal, call_back,visited);
            auto tmp = new APointerType();
            tmp->pointsTo = type;
            a_type = tmp;
            break;
        }
        case GET_NAME_TYPE: {
            auto name = fetchString(ins->getArgOperand(0));
            auto tmp = new StubType(name);
            a_type = tmp;
            break;
        }
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
        default:
            errs() <<"the switch should cover everything this is wrong\n";
            errs() << *ins;
            assert(false);
            break;
    }
    analysisTypeMap[ins] = a_type;
    return a_type;
} // endof parseTypeCallInst

std::string ObjectLowering::fetchString(Value* ins) {
    if(auto firstargGep = dyn_cast<GetElementPtrInst>(ins)) {
        if(firstargGep->getNumIndices() == 2 && firstargGep->hasAllZeroIndices()) {
            if (auto glob_var = dyn_cast<GlobalVariable>(firstargGep->getPointerOperand())) {
                if (auto cda = dyn_cast<ConstantDataArray>(glob_var->getInitializer())) {
                    auto str = cda->getAsCString().str();
                    return str;
                }
            }
        }
    }
    errs() << "Not able to fetch str from " << *ins << "\n";
    assert(false);
}

ObjectWrapper *ObjectLowering::parseObjectWrapperChain(Value* i, std::set<PHINode*> &visited)
{
    errs() << "Trying to obtain object wrapper for " << *i <<"\n";
    ObjectWrapper* objw;
    std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
        //errs() << "Field Wrapper found function " << *ci << "\n";
        objw = parseObjectWrapperInstruction(ci,visited);
    };
    parseType(i, call_back,visited);
    return objw;
}

ObjectWrapper *ObjectLowering::parseObjectWrapperInstruction(CallInst *i, std::set<PHINode*> &visited) {
    // return the cached objectWrapper, if it exists
    if (buildObjMap.find(i)!=buildObjMap.end())
    {
        return buildObjMap[i];
    }
    auto funcName = i->getCalledFunction()->getName().str();
    if(funcName == "buildObject"){
        auto typeArg = i->getArgOperand(0); // this should be a loadInst from a global Type**
        AnalysisType* type;
        std::function<void(CallInst*)> callback = [&](CallInst* ci) {
            type = parseTypeCallInst(ci,visited);
        };
        parseType(typeArg,callback,visited);
        //errs() << "Obtained AnalysisType for " << *i <<"\n";

        if(type->getCode() != ObjectTy) {
            //errs() << "It's not an object";
            assert(false);
        }
        auto* objt = (ObjectType*) type;
        buildObjMap[i] = new ObjectWrapper(objt);
    }
    else if(funcName == "readPointer")
    {
        errs() << "Processing the type of the inner pointer for " << *i << "\n\n";
        FieldWrapper* fw;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
            fw = parseFieldWrapperIns(ci,visited);
        };
        parseType(i->getArgOperand(0), call_back,visited);
        buildObjMap[i] = new ObjectWrapper(fw->objectType);
    }
    else
    {
        assert(false);
    }
    return  buildObjMap[i];
}




void ObjectLowering::parseType(Value *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*>& visited) {
   //errs()<<*ins << "is being called by parseType\n";

    if (auto callins = dyn_cast_or_null<CallInst>(ins)) {
        callback(callins);
    }
    else if (auto storeIns = dyn_cast_or_null<StoreInst>(ins)) {
        parseTypeStoreInst(storeIns,callback,visited);
    }
    else if(auto loadIns = dyn_cast_or_null<LoadInst>(ins)) {
        parseTypeLoadInst(loadIns,callback,visited);
    }
    else if (auto allocaIns = dyn_cast_or_null<AllocaInst>(ins)) {
        parseTypeAllocaInst(allocaIns,callback,visited);
    } else if (auto gv = dyn_cast_or_null<GlobalValue>(ins)) {
        parseTypeGlobalValue(gv,callback,visited);
    } else if (auto phiInst = dyn_cast_or_null<PHINode>(ins)) {
        // if the value has been visited, do nothing
        if(visited.find(phiInst) != visited.end()) return;
        // otherwise, vist the children
        visited.insert(phiInst);
        for (auto& val: phiInst->incoming_values()) parseType(val.get(), callback, visited);
    } else if (!ins) {
        //errs() << "i think this is a nullptr\n";
        assert(false); 
    } else {
        errs() << "parseType: Unrecognized instruction" << *ins <<"\n";
        assert(false);
    }
}



void ObjectLowering::parseTypeStoreInst(StoreInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    auto valOp = ins->getValueOperand();
    parseType(dyn_cast_or_null<Instruction>(valOp), callback,visited);
}

void ObjectLowering::parseTypeLoadInst(LoadInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    auto ptrOp = ins->getPointerOperand();
    parseType(ptrOp,callback,visited);
}

void ObjectLowering::parseTypeAllocaInst(AllocaInst *ins, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    for(auto u: ins->users()) {
        if(auto i = dyn_cast_or_null<StoreInst>(u)) {
            return parseType(i,callback,visited);
        }
    }
    //errs() << "Didn't find any store instruction uses for the instruction" <<*ins;
    assert(false);
}

void ObjectLowering::parseTypeGlobalValue(GlobalValue *gv, const std::function<void(CallInst*)>& callback, std::set<PHINode*> &visited) {
    for(auto u: gv->users()) {
        if(auto i = dyn_cast_or_null<StoreInst>(u)) {
            return parseType(i,callback,visited);
        }
    }
    //errs() << "Didn't find any store instruction uses for the gv" << *gv;
    assert(false);
}



FieldWrapper* ObjectLowering::parseFieldWrapperIns(CallInst* i, std::set<PHINode*> &visited)
{
    errs() << "Parsing field wrapper for " << *i << "\n";

    auto callee = i->getCalledFunction();
    if (!callee) {
        errs() << "Unrecognized indirect call" << *i << "\n";
        assert(false);
    }
    auto n = callee->getName().str();
    if(n != "getObjectField") {
        errs() << "Def use chain gave us the wrong function?" << *i;
        assert(false);
    }
    // get the arguments out of @getObjectField
    auto objPtr = i->getArgOperand(0);
    auto llvmFieldIndex =  i->getArgOperand(1);
    auto CI = dyn_cast_or_null<ConstantInt>(llvmFieldIndex);
    assert(CI);
    int64_t fieldIndex = CI->getSExtValue();
    // grab the ObjectWrapper*
    auto objw = parseObjectWrapperChain(objPtr, visited);
    errs() << "Obtained Field Wrapper AnalysisType for " << *i <<"\n";
    auto fieldwrapper = new FieldWrapper();
    fieldwrapper->baseObjPtr = objPtr;
    fieldwrapper->fieldIndex = fieldIndex; // NOLINT(cppcoreguidelines-narrowing-conversions)
    fieldwrapper->objectType = objw->innerType; 
    return fieldwrapper;
}

// ============================= TRANSFORMATION ===========================================

void ObjectLowering::transform() {
    errs() << "\n Starting transformation\n\n";
    for (auto f : functionsToProcess) {
        // clear these maps for every function
        replacementMapping.clear();
        phiNodesToPopulate.clear();

        DominatorTree &DT = mp->getAnalysis<DominatorTreeWrapperPass>(*f).getDomTree();
        auto &entry = f->getEntryBlock();

        // traverse the dominator to replace instructions
        BasicBlockTransformer(DT,&entry);

        // repopulate incoming values of phi nodes
        for (auto old_phi : phiNodesToPopulate) {
            if (replacementMapping.find(old_phi) == replacementMapping.end()) {
                // errs() << "obj_lowering transform: no new phi found for " << *old_phi << "\n";
                assert(false);
            }
            auto new_phi = dyn_cast<PHINode>(replacementMapping[old_phi]);
            assert(new_phi);
            // errs() << "will replace `" << *old_phi << "` with: `" << *new_phi << "\n";
            for (size_t i = 0; i < old_phi->getNumIncomingValues(); i++) {
                auto old_val = old_phi->getIncomingValue(i);
                if (replacementMapping.find(old_val) == replacementMapping.end()) {
                    // errs() << "obj_lowering transform: no new inst found for " << *old_val << "\n";
                    assert(false);
                }
                auto new_val = replacementMapping[old_val];
                // errs() << "Replacing operand" << i << ": " << *old_val << " with: " << *new_val << "\n";
                new_phi->addIncoming(new_val, old_phi->getIncomingBlock(i));
            }
            // errs() << "finished populating " << *new_phi << "\n";
        }

        // DELETE OBJECT IR INSTRUCTIONS
        std::set<Value*> toDelete;
        // start with instructions we already replaced
        for (auto p : replacementMapping) toDelete.insert(p.first);
        // recursively find all instructions to delete 
        for (auto p : replacementMapping) findInstsToDelete(p.first, toDelete);

        //errs() << "ObjectLowing: deleting the following instructions\n";
        for (auto v : toDelete) {
            //errs() << *v << "\n";
            if (auto i = dyn_cast<Instruction>(v)) { 
                i->replaceAllUsesWith(UndefValue::get(i->getType()));       
                i->eraseFromParent();
            }
        }
    }
    
} // endof transform

void ObjectLowering::BasicBlockTransformer(DominatorTree &DT, BasicBlock *bb)
{
    //errs() << "Transforming Basic Block  " <<*bb << "\n\n";
    // setup llvm::type and malloc function constants
    auto &ctxt = M.getContext();
    auto int64Ty = llvm::Type::getInt64Ty(ctxt);
    auto int32Ty = llvm::Type::getInt32Ty(ctxt);
    auto i8Ty = llvm::IntegerType::get(ctxt, 8);
    auto i8StarTy = llvm::PointerType::getUnqual(i8Ty);
    auto funcType = llvm::FunctionType::get(i8StarTy, ArrayRef<Type *>({ int64Ty }), false);
    auto mallocf = M.getOrInsertFunction("malloc", funcType);

    for(auto &ins: *bb)
    {
        //errs() << "encountering  instruction " << ins <<"\n";
        IRBuilder<> builder(&ins);
        if(auto phi = dyn_cast<PHINode>(&ins))
        {
            //errs()<< "The phi has type " << *phi->getType() <<"\n";
            //errs() << "our type is " << *llvmObjectType << "\n";
            if(phi->getType() == llvmObjectType)
            {
                //errs() << "those two types as equal" <<"\n";
                std::set<PHINode*> visited;
                ObjectWrapper* objw = parseObjectWrapperChain(phi,visited);
                auto llvmType = objw->innerType->getLLVMRepresentation(M);
                auto llvmPtrType = PointerType::getUnqual(llvmType);
                auto newPhi = builder.CreatePHI(llvmPtrType, phi->getNumIncomingValues() );
                //errs() << "out of the old phi a new one is born" << *newPhi <<"\n";
                phiNodesToPopulate.insert(phi);
                replacementMapping[phi] = newPhi;
            }
        }
        else if(auto callIns = dyn_cast<CallInst>(&ins))
        {
            auto callee = callIns->getCalledFunction();
            if(callee == nullptr) continue;
            auto calleeName = callee->getName().str();
            if (! isObjectIRCall(calleeName)) continue;

            switch (FunctionNamesToObjectIR[calleeName]) {
            case BUILD_OBJECT:
            {
                // create malloc based on the object's LLVMRepresentation ; bitcast to a ptr to LLVMRepresentation
                auto llvmType = buildObjMap[callIns]->innerType->getLLVMRepresentation(M);
                auto llvmTypeSize = llvm::ConstantInt::get(int64Ty, M.getDataLayout().getTypeAllocSize(llvmType));
                std::vector<Value *> arguments{llvmTypeSize};
                auto newMallocCall = builder.CreateCall(mallocf ,arguments);

                auto bc_inst = builder.CreateBitCast(newMallocCall, PointerType::getUnqual(llvmType));

                replacementMapping[callIns] = bc_inst;
                break;
            }
            case WRITE_UINT64: {
                auto fieldWrapper = readWriteFieldMap[callIns];
                auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder);
                auto storeInst = builder.CreateStore(callIns->getArgOperand(1),gep);
                replacementMapping[callIns] = storeInst;
                //errs() << "out of the write gep is born" << *gep <<"\n";
                //errs() << "out of the gep a store is born" << *storeInst <<"\n";
                break;
            }
            case READ_UINT64: {
                auto fieldWrapper = readWriteFieldMap[callIns];
                auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder);
                auto loadInst = builder.CreateLoad(int64Ty,gep, "loadfrominst64");
                replacementMapping[callIns] = loadInst;
                ins.replaceAllUsesWith(loadInst);
                //errs() << "out of the write gep is born" << *gep <<"\n";
                //errs() << "from the readuint64 we have a load" << *loadInst <<"\n";
                break;
            } 
            case READ_POINTER: {
                // create gep. load i8* from the gep. bitcast the load to a ptr to LLVMRepresentation
                auto fieldWrapper = readWriteFieldMap[callIns];
                auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder);
                auto loadInst = builder.CreateLoad(i8StarTy,gep, "loadfromPtr");
                // fetch the Type*, which should be a PointerTy/APointerType
                auto refPtr = fieldWrapper->objectType->fields[fieldWrapper->fieldIndex]; 
                if(refPtr->getCode() != PointerTy) {
                    errs() << "BBTransform: " << refPtr->toString() << "not a pointer\n\n";
                    assert(false);
                }
                // the pointsTo must be an ObjectType, which we can use to get the target type for bitcast
                auto objTy = ((APointerType*) refPtr)->pointsTo;
                if (objTy->getCode() != ObjectTy) {
                    errs() << "BBTransform: " << objTy->toString() << "not an object\n\n";
                    assert(false);
                }
                auto llvmtype = ((ObjectType*) objTy)->getLLVMRepresentation(M);
                auto bc_inst = builder.CreateBitCast(loadInst, PointerType::getUnqual(llvmtype));
                replacementMapping[callIns] = bc_inst;
                break;

            } 
             case WRITE_POINTER: {
                 // create gep. bitcast the value-to-be-written into i8*. store the bitcast
                auto fieldWrapper = readWriteFieldMap[callIns];
                auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder);
                auto new_val = callIns->getArgOperand(1);
                if (replacementMapping.find(new_val) == replacementMapping.end()) {
                    errs() << "BBtransform: no replacement found for value: " << callIns << "\n";
                    assert(false);
                }
                auto replPtr = replacementMapping[new_val];
                auto bc_inst = builder.CreateBitCast(replPtr, i8StarTy);
                auto storeInst = builder.CreateStore(bc_inst, gep);
                replacementMapping[callIns] = storeInst;
                break;
            }
            default: continue;
            } // endof switch
        }
    }

    // transform the children in dominator-order
    auto node = DT.getNode(bb);
    for(auto child: node->getChildren())
    {
        auto dominated = child->getBlock();
        BasicBlockTransformer(DT, dominated);
    }
} // endof BasicBlockTransformer

Value* ObjectLowering::CreateGEPFromFieldWrapper(FieldWrapper* fieldWrapper, IRBuilder<>& builder) {
    auto int32Ty = llvm::Type::getInt32Ty(M.getContext());
    errs() << "field wrappere " <<fieldWrapper;
    errs() << "Field Wrapper Base "<< fieldWrapper->baseObjPtr;
    errs() << "Field Wrapper obj type "<< fieldWrapper->objectType;
    errs() << "Field Wrapper index "<< fieldWrapper->fieldIndex;
    auto llvmType = fieldWrapper->objectType->getLLVMRepresentation(M);
    std::vector<Value*> indices = {llvm::ConstantInt::get(int32Ty, 0),
                                   llvm::ConstantInt::get(int32Ty,fieldWrapper->fieldIndex )};
    if(replacementMapping.find(fieldWrapper->baseObjPtr) == replacementMapping.end()) {
        //errs() << "unable to find the base pointer " << *fieldWrapper->baseObjPtr <<"\n";
        assert(false);
    }
    auto llvmPtrType = PointerType::getUnqual(llvmType);
    auto gep = builder.CreateGEP(replacementMapping[fieldWrapper->baseObjPtr],indices);
    return gep;
}

void ObjectLowering::findInstsToDelete(Value* i, std::set<Value*> &toDelete) {
    for (auto u : i->users()) {
        if (toDelete.find(u) != toDelete.end()) continue;
        toDelete.insert(u);
        findInstsToDelete(u, toDelete);
    }
}
