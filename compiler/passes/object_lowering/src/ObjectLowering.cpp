#include "ObjectLowering.hpp"
#include "types.hpp"
#include <functional>

using namespace object_lowering;

ObjectLowering::ObjectLowering(Module &M, Noelle *noelle, ModulePass *mp)
        : M(M),
          noelle(noelle),
          mp(mp) {
    // Do initialization.
    this->parser = new Parser(M, noelle, mp);

    // get llvm::Type* for ObjectIR::Object*
    auto getbuildObjFunc = M.getFunction(ObjectIRToFunctionNames[BUILD_OBJECT]);
    if (!getbuildObjFunc) {
        errs() << "Failed to retrieve Object*";
        assert(false);
    }
    this->object_star = getbuildObjFunc->getReturnType();

    // get the LLVM::Type* for ObjectIR::Type*
    auto getTypeFunc = M.getFunction(ObjectIRToFunctionNames[OBJECT_TYPE]);
    if (!getTypeFunc) {
        getTypeFunc = M.getFunction(ObjectIRToFunctionNames[NAME_OBJECT_TYPE]);
        if (!getTypeFunc) {
            errs() << "Failed to retrieve Type*";
            assert(false);
        }
    }
    this->type_star = getTypeFunc->getReturnType();
    this->type_star_star = PointerType::getUnqual(type_star);
}

void ObjectLowering::analyze() {
    cacheTypes();

    // determine which functions to clone by scanning function type signatures for instances of Object*
    std::vector<Function *> functions_to_clone;
    for (auto &F: M) {
        if (F.isDeclaration()) continue;
        auto ft = F.getFunctionType();
        bool should_clone = false;
        for (auto &paramType: ft->params()) {
            if (paramType == object_star) {
                should_clone = true;
                break;
            }
        }
        if (ft->getReturnType() == object_star) should_clone = true;
        if (should_clone)  functions_to_clone.push_back(&F);
    }

    // clone the functions
    std::map<Function*, ObjectType*> clonedFunctionReturnTypes;
    for (auto &oldF: functions_to_clone) {
        // get arguments types
        vector<Type *> ArgTypes;
        inferArgTypes(oldF, &ArgTypes);
        
        // get return type
        Type *retTy;
        auto ft = oldF->getFunctionType();
        if (ft->getReturnType() == object_star) {
            auto objt = inferReturnType(oldF);
            retTy = llvm::PointerType::getUnqual(objt->getLLVMRepresentation(M));
            clonedFunctionReturnTypes[oldF] = objt;
        } else {
            retTy = ft->getReturnType();
        }       

        // create and fill the new function
        FunctionType *FTy = FunctionType::get(retTy, ArgTypes, oldF->getFunctionType()->isVarArg());
        Function *newF = Function::Create(FTy, oldF->getLinkage(), oldF->getAddressSpace(),
                                          oldF->getName(), oldF->getParent());
        newF->getBasicBlockList().splice(newF->begin(), oldF->getBasicBlockList());

        // construct a map from old to new args
        map<Argument *, Argument *> old_to_new;
        auto new_arg_itr = newF->arg_begin();
        for (auto old_arg_itr = oldF->arg_begin(); old_arg_itr != oldF->arg_end(); ++old_arg_itr) {
            old_to_new[&*old_arg_itr] = &*new_arg_itr;
            new_arg_itr++;
        }

        clonedFunctionMap[oldF] = newF;

        functionArgumentMaps[newF] = old_to_new;
        
    }

    parser->setClonedFunctionReturnTypes(clonedFunctionReturnTypes);

} // endof analyze



void ObjectLowering::cacheTypes() {
    errs() << "\n\nRunning ObjectLowering::cacheTypes\n";

    // collect all GlobalVals which are Type*
    std::vector<GlobalValue *> typeDefs;
    for (auto &globalVar: M.getGlobalList()) {
        if (globalVar.getType() == type_star_star) {
            typeDefs.push_back(&globalVar);
        }
    }

    std::map<string, AnalysisType *> namedTypeMap;
    std::vector<AnalysisType *> allTypes;

    // parse the Types
    for (auto v: typeDefs) {
        object_lowering::AnalysisType *type;
        std::set<PHINode *> visited;
        std::function<void(CallInst *)> call_back = [&](CallInst *ci) {
            type = parser->parseTypeCallInst(ci, visited);
        };
        parser->parseType(v, call_back, visited);

        allTypes.push_back(type);
        if (type->getCode() == ObjectTy) {
            auto objTy = (ObjectType *) type;
            if (objTy->hasName()) namedTypeMap[objTy->name] = objTy;
        }
    }
    // assume that 1) all named Types ARE global vals and not intermediate
    // 2) all un-named types are also global vals

    // resolve stubs
    for (auto type: allTypes) {
        if (type->getCode() == ObjectTy) {
            auto objTy = (ObjectType *) type;
            for (auto fld: objTy->fields) {
                if (fld->getCode() == PointerTy) {
                    auto ptrTy = (APointerType *) fld;
                    auto pointsTo = ptrTy->pointsTo;
                    if (pointsTo->getCode() == StubTy) {
                        auto stubTy = (StubType *) pointsTo;
                        ptrTy->pointsTo = namedTypeMap[stubTy->name];
                    }
                }
            }
        }
    }

    for (auto v: allTypes) {
        errs() << v->toString() << "\n\n";
    }
    errs() << "Done caching types\n\n";
} // endof cachetypes


void ObjectLowering::inferArgTypes(llvm::Function *f, vector<Type *> *arg_vector) {
    auto ft = f->getFunctionType();
    auto args = f->arg_begin();
    for (auto ogType: ft->params()) {
        if (ogType == object_star) {
            // this argument is an Object* => scan the users for assertType()
            auto &argi = *args;
            for (auto u: argi.users()) {
                if (auto ins = dyn_cast_or_null<CallInst>(u)) {
                    auto callee = ins->getCalledFunction();
                    if (!callee) continue;
                    auto n = callee->getName().str();
                    if (n == ObjectIRToFunctionNames[ASSERT_TYPE]) {
                        // use parseType to retreive the type info from the first operand
                        auto newTypeInst = ins->getArgOperand(0);
                        object_lowering::AnalysisType *a_type;
                        std::set<PHINode *> visited;
                        std::function<void(CallInst *)> call_back = [&](CallInst *ci) {
                            a_type = parser->parseTypeCallInst(ci, visited);
                        };
                        parser->parseType(newTypeInst, call_back, visited);
                        // make sure it is an ObjectType
                        assert(a_type);
                        if (a_type->getCode() != ObjectTy) assert(false);
                        auto *objt = (ObjectType *) a_type;
                        auto llvm_type = objt->getLLVMRepresentation(M);
                        arg_vector->push_back(llvm::PointerType::getUnqual(llvm_type)); // turn this obj into pointer
                    }
                }
            }
        } else {
            arg_vector->push_back(ogType);
        }
        args++;
    }
}

ObjectType *ObjectLowering::inferReturnType(llvm::Function *f) {
    for (auto &bb: *f) {
        for (auto &ins: bb) {
            if (auto callIns = dyn_cast<CallInst>(&ins)) {
                auto callee = callIns->getCalledFunction();
                if (callee == nullptr) continue;
                auto calleeName = callee->getName().str();
                if (!isObjectIRCall(calleeName)) continue;
                if (FunctionNamesToObjectIR[calleeName] == SET_RETURN_TYPE) {
                    // use parseType to retreive the type info from the first operand
                    auto newTypeInst = callIns->getArgOperand(0);
                    object_lowering::AnalysisType *a_type;
                    std::set<PHINode *> visited;
                    std::function<void(CallInst *)> call_back = [&](CallInst *ci) {
                        a_type = parser->parseTypeCallInst(ci, visited);
                    };
                    parser->parseType(newTypeInst, call_back, visited);
                    // make sure it is an ObjectType
                    assert(a_type);
                    if (a_type->getCode() != ObjectTy) assert(false);
                    auto *objt = (ObjectType *) a_type;
                    return objt;
                }
            }
        }
    }
    errs() << "did not find setReturnType\n";
    assert(false);
}

// ============================ EXPERIMENTAL =============================================

void ObjectLowering::loopstructure(){

    auto mainF = M.getFunction("main"); // HACK
    auto loopStructures = noelle->getLoopStructures(mainF);
    auto loopForest = noelle->organizeLoopsInTheirNestingForest(*loopStructures);

    for (auto loopTree: loopForest->getTrees())
    {
        auto rootLoop = loopTree->getLoop();
        for (auto latch: rootLoop->getLatches())
        {
            errs() << "latch: " << *latch << "\n\n\n\n";
        }
    }
}


DataFlowResult * ObjectLowering::dataflow(Function *f, std::set<CallInst *> &buildObjs) {
    auto dfe = noelle->getDataFlowEngine();

    auto computeGEN = [&](Instruction *i, DataFlowResult *df) {
        if (!isa<CallInst>(i)) return;
        auto callIns = dyn_cast<CallInst>(i);
        auto callee = callIns->getCalledFunction();
        if (!callee) return;
        auto calleeName = callee->getName().str();
        if (!isObjectIRCall(calleeName)) return;
        if (FunctionNamesToObjectIR[calleeName] == BUILD_OBJECT) {
            auto& gen = df->GEN(i);
            gen.insert(i);
            buildObjs.insert(callIns);
        }
        return ;
    };
    auto computeKILL = [](Instruction *i, DataFlowResult *df) {
        if (!isa<CallInst>(i)) return;
        auto callIns = dyn_cast<CallInst>(i);
        auto callee = callIns->getCalledFunction();
        if (!callee) return;
        auto calleeName = callee->getName().str();
        if (!isObjectIRCall(calleeName)) return;
        if (FunctionNamesToObjectIR[calleeName] == DELETE_OBJECT) {
            auto obj = callIns->getArgOperand(0);
            if (auto buildIns = dyn_cast<CallInst>(obj)) {
                auto callee = buildIns->getCalledFunction();
                if (!callee) return;
                auto calleeName = callee->getName().str();
                if (!isObjectIRCall(calleeName)) return;
                if (FunctionNamesToObjectIR[calleeName] == BUILD_OBJECT) {
                    auto& kill = df->KILL(i);
                    kill.insert(buildIns);
                }
            }            
        }
        return ;
    };
    auto initializeIN = [](Instruction *inst, std::set<Value *>& IN) { return; };
    auto initializeOUT = [](Instruction *inst, std::set<Value *>& OUT) { return; };
    auto computeIN = [](Instruction *inst, std::set<Value *>& IN, Instruction *predecessor, DataFlowResult *df) {
        auto& outI = df->OUT(predecessor);
        IN.insert(outI.begin(), outI.end());
        return ;
    };
    auto computeOUT = [](Instruction *inst, std::set<Value *>& OUT, DataFlowResult *df) {
        auto &inI = df->IN(inst);
        auto &genI = df->GEN(inst);
        auto &killI = df->KILL(inst);

        OUT.insert(inI.begin(), inI.end());
        for (auto k : killI) {
          OUT.erase(k);
        } 
        OUT.insert(genI.begin(), genI.end());
    };
    

    auto customDfr = dfe.applyForward(
        f,
        computeGEN, 
        computeKILL, 
        initializeIN,
        initializeOUT,
        computeIN, 
        computeOUT
    );


    return customDfr;
}


// ============================= TRANSFORMATION ===========================================

void ObjectLowering::transform()
{
    for(auto &f : M)
    {
        if(!f.isDeclaration())
        {
            FunctionTransform(&f);
        }
    }
    // TODO: delete GVs and users
}

void ObjectLowering::FunctionTransform(Function *f) {
    errs() << "\n Starting transformation on " << f->getName() << "\n\n";

    std::map<Value *, Value *> replacementMapping;
    std::set<PHINode *> phiNodesToPopulate;

    DominatorTree &DT = mp->getAnalysis<DominatorTreeWrapperPass>(*f).getDomTree();
    auto &entry = f->getEntryBlock();

    // if this function is a clone, we need to populate the replacementMapping with its arguments
    if (functionArgumentMaps.find(f) != functionArgumentMaps.end()) {
        for (const auto &p: functionArgumentMaps[f]) {
            replacementMapping[p.first] = p.second;
        }
    }

    std::set<CallInst*> buildObjs;
    auto dataflowResult = dataflow(f, buildObjs);

    std::set<Value*> liveBuildObjs;
    for (auto &BB : *f) {
        auto term = BB.getTerminator();
        if (!isa<ReturnInst>(term)) continue;
        auto insts = dataflowResult->OUT(term);
        for (auto possibleInst : insts){
            liveBuildObjs.insert(possibleInst);
        }
    }

    auto loopStructures = noelle->getLoopStructures(f);

    std::set<CallInst*> allocBuildObjects;

    for(auto buildObjins: buildObjs)
    {
        if(liveBuildObjs.find(buildObjins)!=liveBuildObjs.end())
        {
            continue;
        }
        bool inLoop = false;
        for(auto loop: *loopStructures)
        {
            if(!loop->isIncluded(buildObjins))
            {
                continue;
            }
            inLoop=true;
            for(auto loopLatches: loop->getLatches())
            {
                auto lastIns = &(loopLatches->back());
                auto& latchOut = dataflowResult->OUT(lastIns);
                if(latchOut.find(buildObjins) == latchOut.end())
                {
                    allocBuildObjects.erase(buildObjins);
                    goto buildObjectLive;
                }
                allocBuildObjects.insert(buildObjins);
            }
        }
        if(!inLoop)
        {
            allocBuildObjects.insert(buildObjins);
        }

buildObjectLive:
        continue;
        ///
    }
    Instruction* entryIns = entry.getFirstNonPHI();
    IRBuilder<> builder(entryIns);
    for(auto allocaBuildObj: allocBuildObjects)
    {
        std::set<PHINode*> visited;
        auto objT = parser->parseObjectWrapperChain(allocaBuildObj,visited);
        auto llvmType =objT->innerType->getLLVMRepresentation(M);
        auto allocaIns = builder.CreateAlloca(llvmType);
        replacementMapping[allocaBuildObj] = allocaIns;
        errs() << *allocaBuildObj << " will be replaced by " << *allocaInst << "\n";
    }



    // traverse the dominator to replace instructions
    BasicBlockTransformer(DT, &entry, replacementMapping, phiNodesToPopulate, allocBuildObjects);

    // repopulate incoming values of phi nodes
    for (auto old_phi: phiNodesToPopulate) {
        if (replacementMapping.find(old_phi) == replacementMapping.end()) {
            errs() << "obj_lowering transform: no new phi found for " << *old_phi << "\n";
            assert(false);
        }
        auto new_phi = dyn_cast<PHINode>(replacementMapping[old_phi]);
        assert(new_phi);
        // errs() << "will replace `" << *old_phi << "` with: `" << *new_phi << "\n";
        for (size_t i = 0; i < old_phi->getNumIncomingValues(); i++) {
            auto old_val = old_phi->getIncomingValue(i);
            if (replacementMapping.find(old_val) == replacementMapping.end()) {
                errs() << "obj_lowering transform: no new inst found for " << *old_val << "\n";
                assert(false);
            }
            auto new_val = replacementMapping[old_val];
            // errs() << "Replacing operand" << i << ": " << *old_val << " with: " << *new_val << "\n";
            new_phi->addIncoming(new_val, old_phi->getIncomingBlock(i));
        }
        // errs() << "finished populating " << *new_phi << "\n";
    }

    // DELETE OBJECT IR INSTRUCTIONS
    std::set<Value *> toDelete;
    // start with instructions we already replaced
    for (auto p: replacementMapping) toDelete.insert(p.first);
    // recursively find all instructions to delete
    for (auto p: replacementMapping) findInstsToDelete(p.first, toDelete);

    //errs() << "ObjectLowing: deleting the following instructions\n";
    for (auto v: toDelete) {
        //errs() << *v << "\n";
        if (auto i = dyn_cast<Instruction>(v)) {
            i->replaceAllUsesWith(UndefValue::get(i->getType()));
            i->eraseFromParent();
        }
    }


} // endof transform

void ObjectLowering::BasicBlockTransformer(DominatorTree &DT, BasicBlock *bb,
                                           std::map<Value *, Value *> &replacementMapping,
                                           std::set<PHINode *> &phiNodesToPopulate,
                                           std::set<CallInst*> &allocaBuildObj) {
    //errs() << "Transforming Basic Block  " <<*bb << "\n\n";
    // setup llvm::type and malloc function constants
    auto &ctxt = M.getContext();
    auto int64Ty = llvm::Type::getInt64Ty(ctxt);
    auto int32Ty = llvm::Type::getInt32Ty(ctxt);
    auto i8Ty = llvm::IntegerType::get(ctxt, 8);
    auto i8StarTy = llvm::PointerType::getUnqual(i8Ty);
    auto voidTy = llvm::Type::getVoidTy(ctxt);
    auto mallocFTY = llvm::FunctionType::get(i8StarTy, ArrayRef<Type *>({int64Ty}), false);
    auto mallocf = M.getOrInsertFunction("malloc", mallocFTY);
    auto freeFTY = llvm::FunctionType::get(voidTy, ArrayRef<Type *>({i8StarTy}), false);
    auto freef = M.getOrInsertFunction("free", freeFTY);

    for (auto &ins: *bb) {
        //errs() << "encountering  instruction " << ins <<"\n";
        IRBuilder<> builder(&ins);
        if (auto phi = dyn_cast<PHINode>(&ins)) {
            //errs()<< "The phi has type " << *phi->getType() <<"\n";
            //errs() << "our type is " << *llvmObjectType << "\n";
            if (phi->getType() == object_star) {
                //errs() << "those two types as equal" <<"\n";
                std::set<PHINode *> visited;
                ObjectWrapper *objw = parser->parseObjectWrapperChain(phi, visited);
                auto llvmType = objw->innerType->getLLVMRepresentation(M);
                auto llvmPtrType = PointerType::getUnqual(llvmType);
                auto newPhi = builder.CreatePHI(llvmPtrType, phi->getNumIncomingValues());
                //errs() << "out of the old phi a new one is born" << *newPhi <<"\n";
                phiNodesToPopulate.insert(phi);
                replacementMapping[phi] = newPhi;
            }
        } else if (auto callIns = dyn_cast<CallInst>(&ins)) {
            auto callee = callIns->getCalledFunction();
            if (callee == nullptr) continue;
            auto calleeName = callee->getName().str();
            if (!isObjectIRCall(calleeName))
            {
                if(clonedFunctionMap.find(callee) == clonedFunctionMap.end() )
                {
                    continue;
                }
                // this is a function call which passes or returns Object*s
                // replace the arguments
                std::vector<Value *> arguments;
                for(auto &arg: callIns->args())
                {
                    auto val = arg.get();
                    if(replacementMapping.find(val)!=replacementMapping.end())
                    {
                        arguments.push_back(replacementMapping[val]);
                    }
                    else
                    {
                        arguments.push_back(val);
                    }
                }
                auto new_callins = builder.CreateCall(clonedFunctionMap[callee], arguments );
                // if the return type isn't Object*, then we assume it is an intrinsic and its uses must be replaced
                if(callIns->getType() != this->object_star)
                {
                    assert(new_callins->getType() == callIns->getType());
                    ins.replaceAllUsesWith(  new_callins);
                }
                replacementMapping[callIns] = new_callins;

            }else {

                switch (FunctionNamesToObjectIR[calleeName]) {
                    case BUILD_OBJECT: {
                        // create malloc based on the object's LLVMRepresentation ; bitcast to a ptr to LLVMRepresentation
                        if(allocaBuildObj.find(callIns) == allocaBuildObj.end())
                        {
                            continue;
                        }
                        std::set<PHINode*> visited;
                        auto objT = parser->parseObjectWrapperChain(callIns,visited);
                        auto llvmType =objT->innerType->getLLVMRepresentation(M);
                        auto llvmTypeSize = llvm::ConstantInt::get(int64Ty,
                                                                   M.getDataLayout().getTypeAllocSize(llvmType));
                        std::vector<Value *> arguments{llvmTypeSize};
                        auto newMallocCall = builder.CreateCall(mallocf, arguments);

                        auto bc_inst = builder.CreateBitCast(newMallocCall, PointerType::getUnqual(llvmType));

                        replacementMapping[callIns] = bc_inst;
                        break;
                    }
                    case WRITE_UINT64: {
                        std::set<PHINode *> visited;
                        auto fieldWrapper = parser->parseFieldWrapperChain(callIns->getArgOperand(0), visited);
                        auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder, replacementMapping);
                        auto storeInst = builder.CreateStore(callIns->getArgOperand(1), gep);
                        replacementMapping[callIns] = storeInst;
                        //errs() << "out of the write gep is born" << *gep <<"\n";
                        //errs() << "out of the gep a store is born" << *storeInst <<"\n";
                        break;
                    }
                    case READ_UINT64: {
                        std::set<PHINode *> visited;
                        auto fieldWrapper = parser->parseFieldWrapperChain(callIns->getArgOperand(0), visited);
                        auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder, replacementMapping);
                        auto loadInst = builder.CreateLoad(int64Ty, gep, "loadfrominst64");
                        replacementMapping[callIns] = loadInst;
                        ins.replaceAllUsesWith(loadInst);
                        //errs() << "out of the write gep is born" << *gep <<"\n";
                        //errs() << "from the readuint64 we have a load" << *loadInst <<"\n";
                        break;
                    }
                    case READ_POINTER: {
                        // create gep. load i8* from the gep. bitcast the load to a ptr to LLVMRepresentation
                        std::set<PHINode *> visited;
                        auto fieldWrapper = parser->parseFieldWrapperChain(callIns->getArgOperand(0), visited);
                        auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder, replacementMapping);
                        auto loadInst = builder.CreateLoad(i8StarTy, gep, "loadfromPtr");
                        // fetch the Type*, which should be a PointerTy/APointerType
                        auto refPtr = fieldWrapper->objectType->fields[fieldWrapper->fieldIndex];
                        if (refPtr->getCode() != PointerTy) {
                            errs() << "BBTransform: " << refPtr->toString() << "not a pointer\n\n";
                            assert(false);
                        }
                        // the pointsTo must be an ObjectType, which we can use to get the target type for bitcast
                        auto objTy = ((APointerType *) refPtr)->pointsTo;
                        if (objTy->getCode() != ObjectTy) {
                            errs() << "BBTransform: " << objTy->toString() << "not an object\n\n";
                            assert(false);
                        }
                        auto llvmtype = ((ObjectType *) objTy)->getLLVMRepresentation(M);
                        auto bc_inst = builder.CreateBitCast(loadInst, PointerType::getUnqual(llvmtype));
                        replacementMapping[callIns] = bc_inst;
                        break;

                    }
                    case WRITE_POINTER: {
                        // create gep. bitcast the value-to-be-written into i8*. store the bitcast
                        std::set<PHINode *> visited;
                        auto fieldWrapper = parser->parseFieldWrapperChain(callIns->getArgOperand(0), visited);
                        auto gep = CreateGEPFromFieldWrapper(fieldWrapper, builder, replacementMapping);
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
                    case DELETE_OBJECT: {

                        auto arg0 = callIns->getArgOperand(0);
                        if(allocaBuildObj.find(dyn_cast<CallInst>(arg0)) == allocaBuildObj.end())
                        {
                            continue;
                        }
                        // bitcast to i8* and call free
                        auto obj_inst = replacementMapping[arg0];
                        auto bc_inst = builder.CreateBitCast(obj_inst, i8StarTy);
                        std::vector<Value *> arguments{bc_inst};
                        auto free_inst = builder.CreateCall(freef, arguments);
                        replacementMapping[callIns] = free_inst;
                        break;
                    }
                    default:

                        continue;
                } // endof switch
            }
        }
        else if(auto retIns = dyn_cast<ReturnInst>(&ins))
        {
            // replace returned value, if necessary
            auto r_val = retIns->getReturnValue();
            if(replacementMapping.find(r_val) != replacementMapping.end())
            {
                auto new_ret = builder.CreateRet(replacementMapping[r_val]);
                replacementMapping[retIns] = new_ret;
            }
        }
    }

    // transform the children in dominator-order
    auto node = DT.getNode(bb);
    for (auto child: node->getChildren()) {
        auto dominated = child->getBlock();
        BasicBlockTransformer(DT, dominated, replacementMapping,phiNodesToPopulate, allocaBuildObj);
    }
} // endof BasicBlockTransformer

Value *ObjectLowering::CreateGEPFromFieldWrapper(FieldWrapper *fieldWrapper, IRBuilder<> &builder,
                                                 std::map<Value *, Value *> &replacementMapping) {
    auto int32Ty = llvm::Type::getInt32Ty(M.getContext());
    /*errs() << "CreateGEPFromFieldWrapper\n";
    errs() << "\tField Wrapper Base " << *(fieldWrapper->baseObjPtr) << "\n";
    errs() << "\tField Wrapper obj type " << fieldWrapper->objectType->toString() << "\n";
    errs() << "\tField Wrapper index " << fieldWrapper->fieldIndex << "\n";*/
    auto llvmType = fieldWrapper->objectType->getLLVMRepresentation(M);
    std::vector<Value *> indices = {llvm::ConstantInt::get(int32Ty, 0),
                                    llvm::ConstantInt::get(int32Ty, fieldWrapper->fieldIndex)};
    if (replacementMapping.find(fieldWrapper->baseObjPtr) == replacementMapping.end()) {
        errs() << "unable to find the base pointer " << *fieldWrapper->baseObjPtr <<"\n";
        assert(false);
    }
    auto llvmPtrType = PointerType::getUnqual(llvmType);
    auto gep = builder.CreateGEP(replacementMapping.at(fieldWrapper->baseObjPtr), indices);
    return gep;
}

void ObjectLowering::findInstsToDelete(Value *i, std::set<Value *> &toDelete) {
    for (auto u: i->users()) {
        if (toDelete.find(u) != toDelete.end()) continue;
        toDelete.insert(u);
        findInstsToDelete(u, toDelete);
    }
}
