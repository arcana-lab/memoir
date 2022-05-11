#include "ObjectLowering.hpp"
#include "types.hpp"
#include <functional>

using namespace object_lowering;

ObjectLowering::ObjectLowering(Module &M, Noelle *noelle, ModulePass *mp)
  : M(M),
    noelle(noelle),
    mp(mp){
    // Do initialization.
    this->parser = new Parser(M, noelle, mp);

    // get llvm::Type* for ObjectIR::Object*
    auto getbuildObjFunc = M.getFunction(ObjectIRToFunctionNames[BUILD_OBJECT]);
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


    // scan function type signatures for instances of Object*
    std::vector<Function *> functions_to_clone;
    for (auto &F : M) {
        auto ft = F.getFunctionType();
        bool should_clone = false;
        for (auto& paramType : ft->params())
        {
            if(paramType == object_star)
            {
                should_clone = true;
                break;
            }
        }
        if (ft->getReturnType() == object_star) should_clone = true;
        if(should_clone && (!F.isDeclaration()))
        {
            functions_to_clone.push_back(&F);
        }
    }

    // create the cloned functions
    std::map<Function*, Function*> clonedFunctionMap;

    for(auto &oldF: functions_to_clone)
    {
        // Create a new function type...
        vector<Type*> ArgTypes; // this would replace oldF->getFunctionType()->params() below
        inferArgTypes(oldF, &ArgTypes);
        Type* retTy = inferReturnType(oldF);
        errs() << *retTy << " // return type \n";

        FunctionType *FTy = FunctionType::get(oldF->getFunctionType()->getReturnType(), oldF->getFunctionType()->params(), 
                         oldF->getFunctionType()->isVarArg());
        Function *newF = Function::Create(FTy, oldF->getLinkage(), oldF->getAddressSpace(), oldF->getName(), oldF->getParent());
        
        newF->getBasicBlockList().splice(newF->begin(), oldF->getBasicBlockList()); //  THIS BREAKS THE OLD FUNCTION

        tmpPatchup(oldF, newF);
    }




    return; // temporarily skip other code


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
        auto objT = parser->parseObjectWrapperInstruction(ins,visited);
        errs() << "Instruction " << *ins << "\n\n has the type of" << objT->innerType->toString() << "\n\n";
    }


    // construct the FieldWrapper* for each read, write call
    errs() << "READS\n\n";
    for(auto ins : this->reads) {
        errs() << "Parsing: " << *ins << "\n\n";
        FieldWrapper* fw;
        std::set<PHINode*> visited;
        std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
            fw = parser->parseFieldWrapperIns(ci,visited);
        };
        parser->parseType(ins->getArgOperand(0), call_back,visited);
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
            fw = parser->parseFieldWrapperIns(ci,visited);
        };
        parser->parseType(ins->getArgOperand(0), call_back,visited);
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


void ObjectLowering::cacheTypes() {
    errs() << "\n\nRunning ObjectLowering::Analysis\n";

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
            type = parser->parseTypeCallInst(ci,visited);
        };
        parser->parseType(v, call_back,visited);

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
    errs() << "Done caching types\n\n";
} // endof cachetypes


void ObjectLowering::inferArgTypes(llvm::Function* f, vector<Type*> *arg_vector) {
    errs() << "Running inferArgTypes on " << f->getName() << "\n";
    auto ft = f->getFunctionType();
    auto args = f->arg_begin();
    for (auto ogType : ft->params()) {
        if (ogType == object_star) {
            // this argument is an Object* => scan the users for assertType()
            auto &argi = *args;
            for(auto u: argi.users()) {
                if (auto ins = dyn_cast_or_null<CallInst>(u)) {
                    auto callee = ins->getCalledFunction();
                    if (!callee) continue;
                    auto n = callee->getName().str();
                    if (n == ObjectIRToFunctionNames[ASSERT_TYPE]) {
                        // use parseType to retreive the type info from the first operand
                        auto newTypeInst = ins->getArgOperand(0);
                        object_lowering::AnalysisType* a_type;
                        std::set<PHINode*> visited;
                        std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                            a_type = parser->parseTypeCallInst(ci,visited);
                        };
                        parser->parseType(newTypeInst, call_back, visited);
                        // make sure it is an ObjectType
                        assert(a_type);
                        if(a_type->getCode() != ObjectTy) assert(false);
                        auto* objt = (ObjectType*) a_type;
                        auto llvm_type = objt->getLLVMRepresentation(M);
                        arg_vector->push_back(llvm_type);
                    }
                }
            }
        } else {
            arg_vector->push_back(ogType);
        }
        args++;
    }
}

Type* ObjectLowering::inferReturnType(llvm::Function* f) {
    for (auto &bb : *f) {
        for (auto &ins : bb) {
            if(auto callIns = dyn_cast<CallInst>(&ins))
            {
                auto callee = callIns->getCalledFunction();
                if(callee == nullptr) continue;
                auto calleeName = callee->getName().str();
                if (! isObjectIRCall(calleeName)) continue;
                if (FunctionNamesToObjectIR[calleeName] == SET_RETURN_TYPE) {
                    // use parseType to retreive the type info from the first operand
                    auto newTypeInst = callIns->getArgOperand(0);
                    object_lowering::AnalysisType* a_type;
                    std::set<PHINode*> visited;
                    std::function<void(CallInst*)> call_back = [&](CallInst* ci) {
                        a_type = parser->parseTypeCallInst(ci,visited);
                    };
                    parser->parseType(newTypeInst, call_back, visited);
                    // make sure it is an ObjectType
                    assert(a_type);
                    if(a_type->getCode() != ObjectTy) assert(false);
                    auto* objt = (ObjectType*) a_type;
                    auto llvm_type = objt->getLLVMRepresentation(M);
                    return llvm_type;
                }
            }
        }
    }
    errs() << "did not find setReturnType\n";
    assert(false);
}

// ======================================================================

/* this is a proof-of-concept for splicing:
above, we use llvm:Create to make a new function with the SAME type sig s.t. no lowering is required
here, we use the dominator tree of the NEW function to traverse all of its instructions
any use of the OLD arguments are replaced by new arguments
this is proof of concept that splicing and the dominator tree API will work

after that we have to patch up the old call in main
*/
void ObjectLowering::tmpPatchup(Function* oldF, Function* newF) {
    // construct a map from old to new args
    map<Argument*, Argument*> old_to_new;
    auto new_arg_itr = newF->arg_begin();
    for (auto old_arg_itr =  oldF->arg_begin(); old_arg_itr != oldF->arg_end(); ++old_arg_itr) {
        old_to_new[&*old_arg_itr] = &*new_arg_itr;
        new_arg_itr++;
    } 
    // traverse the dom tree
    DominatorTree &DT = mp->getAnalysis<DominatorTreeWrapperPass>(*newF).getDomTree();
    auto entry = &(newF->getEntryBlock());
    tmpDomTreeTraversal(DT, entry, &old_to_new);  
    // patch up the call in main
    Function* mainF = M.getFunction("main");
    for (auto &bb : *mainF) {
        for (auto &ins : bb) {
            if(auto callIns = dyn_cast<CallInst>(&ins)) {
                auto callee = callIns->getCalledFunction();
                if(callee == nullptr) continue;
                auto calleeName = callee->getName().str();
                if (calleeName == oldF->getName()) {
                    callIns->setCalledFunction(newF);
                }
            }
        }
    }

}

void ObjectLowering::tmpDomTreeTraversal(DominatorTree &DT, BasicBlock *bb, map<Argument*, Argument*> *old_to_new) {
    for(auto &ins: *bb) {
        // for test_object_passing, i know that that this argument is only used in OBJECTIR callinsts
        // so i only check those to replace the operands
        if(auto callIns = dyn_cast<CallInst>(&ins))
        {
            auto callee = callIns->getCalledFunction();
            if(callee == nullptr) continue;
            auto calleeName = callee->getName().str();
            if (! isObjectIRCall(calleeName)) continue;
            for (size_t idx = 0; idx < callIns->getNumArgOperands(); idx++) {
                Argument* ciArg = dyn_cast_or_null<Argument>(callIns->getArgOperand(idx));
                if (!ciArg) continue;
                callIns->setArgOperand(idx, old_to_new->at(ciArg));
            }
        }
    }

    auto node = DT.getNode(bb);
    for(auto child: node->getChildren())
    {
        auto dominated = child->getBlock();
        tmpDomTreeTraversal(DT, dominated, old_to_new);
    }
}


// ============================= TRANSFORMATION ===========================================

void ObjectLowering::transform() {
    return; // temporarily skip other code
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
            if(phi->getType() == object_star)
            {
                //errs() << "those two types as equal" <<"\n";
                std::set<PHINode*> visited;
                ObjectWrapper* objw = parser->parseObjectWrapperChain(phi,visited);
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
                auto llvmType = parser->buildObjMap[callIns]->innerType->getLLVMRepresentation(M);
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
