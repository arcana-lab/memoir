#include "ObjectLowering.hpp"
#include <functional>

using namespace llvm;
using namespace llvm::memoir;

namespace object_lowering {
    ObjectLowering::ObjectLowering(Module &M, Noelle *noelle, ModulePass *mp)
            : M(M),
              noelle(noelle),
              mp(mp) {
//   Do initialization.

        // get llvm::Type* for ObjectIR::Object*
        auto allocate_struct_func =
                memoir::getMemOIRFunction(M, memoir::MemOIR_Func::ALLOCATE_STRUCT);
        if (!allocate_struct_func) {
            assert(false && "ObjectLowering::ObjectLowering"
                   && "Failed to retrieve Object*");
        }
        this->object_star = allocate_struct_func->getReturnType();

        // get the LLVM::Type* for ObjectIR::Type*
        auto struct_type_func =
                memoir::getMemOIRFunction(M, memoir::MemOIR_Func::STRUCT_TYPE);
        if (!struct_type_func) {
            auto define_struct_type_func =
                    memoir::getMemOIRFunction(M,
                                              memoir::MemOIR_Func::DEFINE_STRUCT_TYPE);
            if (!define_struct_type_func) {
                assert(false && "ObjectLowering::ObjectLowering"
                       && "Failed to retrieve Type*");
            }
        }

        this->type_star = struct_type_func->getReturnType();
        this->type_star_star = PointerType::getUnqual(type_star);

        auto type_star_as_pointer = dyn_cast<PointerType>(this->type_star);
        if (!type_star_as_pointer) {
            assert(false && "ObjectLowering::ObjectLowering"
                   && "Failed to retrieve Type**");
        }

//        this->parser = new Parser(M, noelle, mp, type_star_as_pointer);
        this->nativeTypeConverter = new NativeTypeConverter(M, noelle);
    }

    void ObjectLowering::analyze() {
//        for (auto &F: M) {
//            if (F.hasName() && F.getName() == "main") {
//                int count = 0;
//                for (auto &i: instructions(F)) {
//                    if (count == 26) {
//                        auto accessIns = dyn_cast<CallInst>(&i);
//                        auto &access_analysis = memoir::AccessAnalysis::get(M);
//                        errs() << *accessIns << "\n";
//                        auto accessSum = access_analysis.getAccessSummary(*accessIns);
//                    }
//
//                    count++;
//                }
//            }
//        }

        // simple testing stuff to make sure it's working

//  auto &allocAna = memoir::AllocationAnalysis::get(M);
//  auto &type_analysis = memoir::TypeAnalysis::get(M);
//  auto &access_analysis = memoir::AccessAnalysis::get(M);
//
//  for (auto &F : M) {
//    if (F.isDeclaration()) {
//      continue;
//    }
//    if (F.hasName() && F.getName().str() == "main") {
//      for (auto &I : instructions(F)) {
//        auto *ins = &I;
//        if (auto callins = dyn_cast<CallInst>(ins)) {
//
//          auto res = allocAna.getAllocationSummary(*callins);
//          errs() << res->toString() << "\n";
//          break;
//
//          //                    memoir::AllocationAnalysis::getAllocationSummary(callins)
//        }
//      }
//    }
//  }

//    cacheTypes();
        //
        //  // determine which functions to clone by scanning function type signatures
        //  for
        //  // instances of Object*
        //  std::vector<Function *> functions_to_clone;
        //  for (auto &F : M) {
        //    if (F.isDeclaration()) {
        //      continue;
        //    }
        //
        //    if (!F.hasName()) {
        //      continue;
        //    }
        //
        //    auto function_name = F.getName().str();
        //    if (function_name.find("main") == std::string::npos) {
        //      continue;
        //    }
        //
        //    auto is_internal =
        //        memoir::MetadataManager::hasMetadata(F,
        //        memoir::MetadataType::INTERNAL);
        //    if (is_internal) {
        //      continue;
        //    }
        //
        //    auto function_type = F.getFunctionType();
        //    bool should_clone = false;
        //    for (auto &param_type : function_type->params()) {
        //      if (param_type == object_star) {
        //        should_clone = true;
        //        break;
        //      }
        //    }
        //
        //    if (function_type->getReturnType() == object_star) {
        //      should_clone = true;
        //    }
        //
        //    if (should_clone) {
        //      functions_to_clone.push_back(&F);
        //    }
        //  }
        //
        //  // clone the functions
        //  std::map<Function *, ObjectType *> clonedFunctionReturnTypes;
        //  for (auto &oldF : functions_to_clone) {
        //    // get arguments types
        //    vector<Type *> arg_types;
        //    inferArgTypes(oldF, &arg_types);
        //
        //    // get return type
        //    Type *retTy;
        //    auto ft = oldF->getFunctionType();
        //    if (ft->getReturnType() == object_star) {
        //      auto objt = inferReturnType(oldF);
        //      retTy = llvm::PointerType::getUnqual(objt->getLLVMRepresentation(M));
        //      clonedFunctionReturnTypes[oldF] = objt;
        //    } else {
        //      retTy = ft->getReturnType();
        //    }
        //
        //    // create and fill the new function
        //    FunctionType *FTy = FunctionType::get(retTy,
        //                                          arg_types,
        //                                          oldF->getFunctionType()->isVarArg());
        //    Function *newF = Function::Create(FTy,
        //                                      oldF->getLinkage(),
        //                                      oldF->getAddressSpace(),
        //                                      oldF->getName(),
        //                                      oldF->getParent());
        //    newF->getBasicBlockList().splice(newF->begin(),
        //    oldF->getBasicBlockList());
        //
        //    // construct a map from old to new args
        //    map<Argument *, Argument *> old_to_new;
        //    auto new_arg_iter = newF->arg_begin();
        //    for (auto old_arg_iter = oldF->arg_begin(); old_arg_iter !=
        //    oldF->arg_end();
        //         ++old_arg_iter) {
        //      old_to_new[&*old_arg_iter] = &*new_arg_iter;
        //      new_arg_iter++;
        //    }
        //
        //    clonedFunctionMap[oldF] = newF;
        //
        //    functionArgumentMaps[newF] = old_to_new;
        //  }
        //
        //  parser->setClonedFunctionReturnTypes(clonedFunctionReturnTypes);

    } // endof analyze

    void ObjectLowering::cacheTypes() {
        //  errs() << "\n\nRunning ObjectLowering::cacheTypes\n";
        //
        // collect all GlobalVals which are Type*
        for (auto &globalVar: M.getGlobalList()) {
            if (globalVar.getType() == type_star_star) {
                typeDefs.push_back(&globalVar);
            }
        }

        //  std::map<string, AnalysisType *> namedTypeMap;
        //  std::vector<AnalysisType *> allTypes;
        //
        //  // parse the Types
        //  for (auto v : typeDefs) {
        //    object_lowering::AnalysisType *type;
        //    std::set<PHINode *> visited;
        //    std::function<void(CallInst *)> call_back = [&](CallInst *ci) {
        //      type = parser->parseTypeCallInst(ci, visited);
        //    };
        //    parser->parseType(v, call_back, visited);
        //
        //    allTypes.push_back(type);
        //    if (type->getCode() == TypeCode::StructTy) {
        //      auto struct_type = (StructTypeSummary *)type;
        //      if (struct_type->hasName()) {
        //        namedTypeMap[struct_type->getName()] = struct_type;
        //      }
        //    }
        //  }
        //  // assume that 1) all named Types ARE global vals and not intermediate
        //  // 2) all un-named types are also global vals
        //
        //  // resolve stubs
        //  for (auto type : allTypes) {
        //    if (type->getCode() == StructTy) {
        //      auto struct_type = (StructTypeSummary *)type;
        //      for (auto field : struct_type->fields) {
        //        if (field->getCode() == ReferenceTy) {
        //          auto ptrTy = (ReferenceTypeSummary *)fld;
        //          auto pointsTo = pointer_type->pointsTo;
        //          if (pointsTo->getCode() == StubTy) {
        //            auto stubTy = (StubType *)pointsTo;
        //            ptrTy->pointsTo = namedTypeMap[stubTy->name];
        //          }
        //        }
        //      }
        //    }
        //  }
        //
        //  for (auto v : allTypes) {
        //    errs() << v->toString() << "\n\n";
        //  }
        //  errs() << "Done caching types\n\n";
    } // endof cachetypes

    void ObjectLowering::inferArgTypes(llvm::Function *f,
                                       vector<Type *> *arg_vector) {
        //  auto ft = f->getFunctionType();
        //  auto args = f->arg_begin();
        //  for (auto ogType : ft->params()) {
        //    if (ogType == object_star) {
        //      // this argument is an Object* => scan the users for assertType()
        //      auto &argi = *args;
        //      for (auto u : argi.users()) {
        //        if (auto ins = dyn_cast_or_null<CallInst>(u)) {
        //          auto callee = ins->getCalledFunction();
        //          if (!callee)
        //            continue;
        //          auto n = callee->getName().str();
        //          if (n == ObjectIRToFunctionNames[ASSERT_TYPE]) {
        //            // use parseType to retreive the type info from the first
        //            operand auto newTypeInst = ins->getArgOperand(0);
        //            object_lowering::AnalysisType *a_type;
        //            std::set<PHINode *> visited;
        //            std::function<void(CallInst *)> call_back = [&](CallInst *ci) {
        //              a_type = parser->parseTypeCallInst(ci, visited);
        //            };
        //            parser->parseType(newTypeInst, call_back, visited);
        //            // make sure it is an ObjectType
        //            assert(a_type);
        //            if (a_type->getCode() != StructTy)
        //              assert(false);
        //            auto *objt = (StructTypeSummary *)a_type;
        ////            objt.
        //            auto llvm_type = objt->getLLVMRepresentation(M);
        //            arg_vector->push_back(llvm::PointerType::getUnqual(
        //                llvm_type)); // turn this obj into pointer
        //          }
        //        }
        //      }
        //    } else {
        //      arg_vector->push_back(ogType);
        //    }
        //    args++;
        //  }
    }
/*
//    ObjectType *ObjectLowering::inferReturnType(llvm::Function *f) {
//        //  errs() << f->getName().str() << ": inferring its return type rn\n";
//        //
//        //  for (auto &bb : *f) {
//        //    for (auto &ins : bb) {
//        //      if (auto callIns = dyn_cast<CallInst>(&ins)) {
//        //        auto callee = callIns->getCalledFunction();
//        //        if (callee == nullptr)
//        //          continue;
//        //        auto calleeName = callee->getName().str();
//        //        if (!isObjectIRCall(calleeName))
//        //          continue;
//        //        if (FunctionNamesToObjectIR[calleeName] == SET_RETURN_TYPE) {
//        //          // use parseType to retreive the type info from the first operand
//        //          auto newTypeInst = callIns->getArgOperand(0);
//        //          object_lowering::AnalysisType *a_type;
//        //          std::set<PHINode *> visited;
//        //          std::function<void(CallInst *)> call_back = [&](CallInst *ci) {
//        //            a_type = parser->parseTypeCallInst(ci, visited);
//        //          };
//        //          parser->parseType(newTypeInst, call_back, visited);
//        //          // make sure it is an ObjectType
//        //          assert(a_type);
//        //          if (a_type->getCode() != ObjectTy)
//        //            assert(false);
//        //          auto *objt = (ObjectType *)a_type;
//        //          return objt;
//        //        }
//        //      }
//        //    }
//        //  }
//        //  errs() << "did not find setReturnType\n";
//        //  assert(false);
//    }
*/

// ============================ STACK VS HEAP
// =============================================

    DataFlowResult *ObjectLowering::dataflow(Function *f,
                                             std::set<CallInst *> &buildObjs) {
        /*
        //  auto dfe = noelle->getDataFlowEngine();
        //
        //  auto computeGEN = [&](Instruction *i, DataFlowResult *df) {
        //    if (!isa<CallInst>(i))
        //      return;
        //    auto callIns = dyn_cast<CallInst>(i);
        //    auto callee = callIns->getCalledFunction();
        //    if (!callee)
        //      return;
        //    auto calleeName = callee->getName().str();
        //    if (!isObjectIRCall(calleeName))
        //      return;
        //    if (FunctionNamesToObjectIR[calleeName] == BUILD_OBJECT) {
        //      auto &gen = df->GEN(i);
        //      gen.insert(i);
        //      buildObjs.insert(callIns); // collect all buildObjs
        //    }
        //    return;
        //  };
        //  auto computeKILL = [](Instruction *i, DataFlowResult *df) {
        //    if (!isa<CallInst>(i))
        //      return;
        //    auto callIns = dyn_cast<CallInst>(i);
        //    auto callee = callIns->getCalledFunction();
        //    if (!callee)
        //      return;
        //    auto calleeName = callee->getName().str();
        //    if (!isObjectIRCall(calleeName))
        //      return;
        //    if (FunctionNamesToObjectIR[calleeName] == DELETE_OBJECT) {
        //      auto obj = callIns->getArgOperand(0);
        //      if (auto buildIns = dyn_cast<CallInst>(obj)) {
        //        auto callee = buildIns->getCalledFunction();
        //        if (!callee)
        //          return;
        //        auto calleeName = callee->getName().str();
        //        if (!isObjectIRCall(calleeName))
        //          return;
        //        if (FunctionNamesToObjectIR[calleeName] == BUILD_OBJECT) {
        //          auto &kill = df->KILL(i);
        //          kill.insert(buildIns);
        //        }
        //      }
        //    }
        //    return;
        //  };
        //  auto initializeIN = [](Instruction *inst, std::set<Value *> &IN) { return;
        //  }; auto initializeOUT = [](Instruction *inst, std::set<Value *> &OUT) {
        //    return;
        //  };
        //  auto computeIN = [](Instruction *inst,
        //                      std::set<Value *> &IN,
        //                      Instruction *predecessor,
        //                      DataFlowResult *df) {
        //    auto &outI = df->OUT(predecessor);
        //    IN.insert(outI.begin(), outI.end());
        //    return;
        //  };
        //  auto computeOUT =
        //      [](Instruction *inst, std::set<Value *> &OUT, DataFlowResult *df) {
        //        auto &inI = df->IN(inst);
        //        auto &genI = df->GEN(inst);
        //        auto &killI = df->KILL(inst);
        //
        //        OUT.insert(inI.begin(), inI.end());
        //        for (auto k : killI) {
        //          OUT.erase(k);
        //        }
        //        OUT.insert(genI.begin(), genI.end());
        //      };
        //
        //  auto customDfr = dfe.applyForward(f,
        //                                    computeGEN,
        //                                    computeKILL,
        //                                    initializeIN,
        //                                    initializeOUT,
        //                                    computeIN,
        //                                    computeOUT);
        //
        //  return customDfr;
         */
    }

// ============================= TRANSFORMATION
// ===========================================

    void ObjectLowering::transform() {
        for (auto &f: M) {
            if (!f.isDeclaration()) {
                auto is_internal =
                        memoir::MetadataManager::hasMetadata(f,
                                                             memoir::MetadataType::INTERNAL);
                if (is_internal) {
                    continue;
                }
                FunctionTransform(&f);
            }
        }

        // delete the Type* global variables
        std::set<Value *> toDelete;
        // start with the GVs
        for (auto p: typeDefs)
            toDelete.insert(p);
        // recursively find all instructions to delete
        for (auto p: typeDefs)
            findInstsToDelete(p, toDelete);

        // errs() << "ObjectLowing: deleting these instructions\n";
        for (auto v: toDelete) {
            // errs() << "\t" << *v << "\n";
            if (auto i = dyn_cast<Instruction>(v)) {
                i->replaceAllUsesWith(UndefValue::get(i->getType()));
                i->eraseFromParent();
            }
        }

        for (GlobalValue *p: typeDefs) {
            // errs() << "Dropping refs: " << *p << "\n";
            p->dropAllReferences();
            // errs() << "\terasing from parent\n";
            p->eraseFromParent();
        }

//        for (auto const &x: clonedFunctionMap) {
//            x.first->eraseFromParent();
//        }

    }

    void ObjectLowering::FunctionTransform(Function *f) {
        errs() << "\n Starting transformation on " << f->getName() << "\n\n";
        //
        std::map<Value *, Value *> replacementMapping;
        std::set<PHINode *> phiNodesToPopulate;

        DominatorTree &DT =
                mp->getAnalysis<DominatorTreeWrapperPass>(*f).getDomTree();
        auto &entry = f->getEntryBlock();
        /*
        // if this function is a clone, we need to populate the replacementMapping
        // with its arguments
        //  if (functionArgumentMaps.find(f) != functionArgumentMaps.end()) {
        //    for (const auto &p : functionArgumentMaps[f]) {
        //      if (p.first->getType() == object_star) {
        //        replacementMapping[p.first] = p.second;
        //      } else {
        //        p.first->replaceAllUsesWith(p.second);
        //      }
        //    }
        //  }
        //
//    std::set<CallInst *> buildObjs;
//    auto dataflowResult = dataflow(f, buildObjs);
        //
        //  std::set<Value *> liveBuildObjs;
        //  for (auto &BB : *f) {
        //    auto term = BB.getTerminator();
        //    if (!isa<ReturnInst>(term))
        //      continue;
        //    auto insts = dataflowResult->OUT(term);
        //    for (auto possibleInst : insts) {
        //      liveBuildObjs.insert(possibleInst);
        //    }
        //  }
        //
        //  for (auto liveObj : liveBuildObjs) {
        //    errs() << "this object is alive " << *liveObj << "\n";
        //  }
        //
        //  errs() << "Getting loop structures \n";
        //  auto loopStructures = noelle->getLoopStructures(f);
        //  errs() << "done getting loop structures\n";
         */
        std::set<CallInst *> allocBuildObjects;
        /*
        //
        //  for (auto buildObjins : buildObjs) {
        //    if (liveBuildObjs.find(buildObjins) != liveBuildObjs.end()) {
        //      continue;
        //    }
        //    bool inLoop = false;
        //    errs() << "about to loop over LS\n";
        //    for (auto loop : *loopStructures) {
        //      errs() << "fetched a loop while checking " << *buildObjins << "\n";
        //      errs() << "This is the loop" << loop << "\n\n\n";
        //      if (!loop->isIncluded(buildObjins)) {
        //        errs() << "This build object is not included in the loop\n";
        //        continue;
        //      }
        //      inLoop = true;
        //      for (auto loopLatches : loop->getLatches()) {
        //        auto lastIns = &(loopLatches->back());
        //
        //        errs()
        //            << "Checking the loop latch with the last ins " << *lastIns <<
        //            "\n";
        //        auto &latchOut = dataflowResult->OUT(lastIns);
        //        if (latchOut.find(buildObjins) != latchOut.end()) {
        //          errs() << "This build object is not dead in one of the latches
        //          \n"; allocBuildObjects.erase(buildObjins); goto buildObjectLive;
        //        }
        //        errs()
        //            << "Adding ins" << *buildObjins << " To the allocBuildObjects
        //            \n";
        //        allocBuildObjects.insert(buildObjins);
        //      }
        //    }
        //    if (!inLoop) {
        //      allocBuildObjects.insert(buildObjins);
        //    }
        //
        //  buildObjectLive:
        //    continue;
        //    ///
        //  }
        //  Instruction *entryIns = entry.getFirstNonPHI();
        //  IRBuilder<> builder(entryIns);
        //  for (auto allocaBuildObj : allocBuildObjects) {
        //    std::set<PHINode *> visited;
        //    auto objT = parser->parseObjectWrapperChain(allocaBuildObj, visited);
        //    auto llvmType = objT->innerType->getLLVMRepresentation(M);
        //    auto allocaIns = builder.CreateAlloca(llvmType);
        //    replacementMapping[allocaBuildObj] = allocaIns;
        //    errs() << *allocaBuildObj << " will be replaced by " << *allocaIns <<
        //    "\n";
        //  }
        //
         */
        errs() << "invoking bbtransform \n";

        // traverse the dominator to replace instructions
        BasicBlockTransformer(DT,
                              &entry,
                              replacementMapping,
                              phiNodesToPopulate,
                              allocBuildObjects);


        // repopulate incoming values of phi nodes
        for (auto old_phi: phiNodesToPopulate) {
            if (replacementMapping.find(old_phi) == replacementMapping.end()) {
                errs() << "obj_lowering transform: no new phi found for " << *old_phi
                       << "\n";
                assert(false);
            }
            auto new_phi = dyn_cast<PHINode>(replacementMapping[old_phi]);
            assert(new_phi);
            // errs() << "will replace `" << *old_phi << "` with: `" << *new_phi <<
            // "\n";
            for (size_t i = 0; i < old_phi->getNumIncomingValues(); i++) {
                auto old_val = old_phi->getIncomingValue(i);
                if (dyn_cast<ConstantPointerNull>(old_val)) {
                    auto new_val =
                            ConstantPointerNull::get(dyn_cast<PointerType>(new_phi->getType()));
                    new_phi->addIncoming(new_val, old_phi->getIncomingBlock(i));
                } else {
                    if (replacementMapping.find(old_val) == replacementMapping.end()) {
                        errs() << "obj_lowering transform: no new inst found for " <<
                               *old_val
                               << "\n";
                        assert(false);
                    }
                    auto new_val = replacementMapping[old_val];
                    errs() << "Replacing operand" << i << ": " << *old_val << " with: " << *new_val << "\n";
                    new_phi->addIncoming(new_val, old_phi->getIncomingBlock(i));
                }
            }
            errs() << "finished populating " << *new_phi << "\n";
        }

        // DELETE OBJECT IR INSTRUCTIONS
//        std::set<Value *> toDelete;
//        // start with instructions we already replaced
//        for (auto p: replacementMapping)
//            toDelete.insert(p.first);
//        // recursively find all instructions to delete
//        for (auto p: replacementMapping)
//            findInstsToDelete(p.first, toDelete);
//
//        // errs() << "ObjectLowing: deleting the following instructions\n";
//        for (auto v: toDelete) {
//            errs() << "deleting: " << *v << "\n";
//            if (auto i = dyn_cast<Instruction>(v)) {
//                i->replaceAllUsesWith(UndefValue::get(i->getType()));
//                i->eraseFromParent();
//            }
//        }

        errs() << *f << "transformed function\n\n";


    } // endof transform

    bool ObjectLowering::isStaticTensor(TensorAllocationSummary* tas, std::vector<int64_t>& sizes)
    {
        for(uint64_t dim = 0; dim < tas->getNumberOfDimensions(); ++dim)
        {
            auto length_dim_val = tas->getLengthOfDimension(dim);
            auto length_dim_constantint = dyn_cast_or_null<ConstantInt>(length_dim_val);
            if(length_dim_constantint == nullptr)
            {
                return false;
            }
            sizes.push_back(length_dim_constantint->getSExtValue());
        }
        return true;
    }

    void ObjectLowering::BasicBlockTransformer(
            DominatorTree &DT,
            BasicBlock *bb,
            std::map<Value *, Value *> &replacementMapping,
            std::set<PHINode *> &phiNodesToPopulate,
            std::set<CallInst *> &allocaBuildObj) {
        // errs() << "Transforming Basic Block  " <<*bb << "\n\n";
        // setup llvm::type and malloc function constants
        auto &ctxt = M.getContext();
        auto int64Ty = llvm::Type::getInt64Ty(ctxt);
        auto int32Ty = llvm::Type::getInt32Ty(ctxt);
        auto i8Ty = llvm::IntegerType::get(ctxt, 8);
        auto i8StarTy = llvm::PointerType::getUnqual(i8Ty);
        auto voidTy = llvm::Type::getVoidTy(ctxt);
        auto mallocFTY =
                llvm::FunctionType::get(i8StarTy, ArrayRef<Type *>({int64Ty}),
                                        false);
        auto mallocf = M.getOrInsertFunction("malloc", mallocFTY);
        auto freeFTY =
                llvm::FunctionType::get(voidTy, ArrayRef<Type *>({i8StarTy}),
                                        false);
        auto freef = M.getOrInsertFunction("free", freeFTY);
        for (auto &ins: *bb) {
            errs() << "encountering  instruction " << ins << "\n";
            IRBuilder<> builder(&ins);
            if (auto phi = dyn_cast<PHINode>(&ins)) {
                errs() << "The phi has type " << *phi->getType() << "\n";
                if (phi->getType() == object_star) {
                    errs() << "and it's an object star" << "\n";
                    std::set<PHINode *> visited;
                    auto &allocAna = memoir::AllocationAnalysis::get(M);
                    auto setsofAlloc = allocAna.getAllocationSummaries(*phi);
                    if (setsofAlloc.empty()) {
                        assert(false && "there is no allocation associated with the phi node");
                    }
                    auto &stype = (*(setsofAlloc.begin()))->getType();
                    //TODO: Ensure type consistency for tensors here
                    auto llvmType = nativeTypeConverter->getLLVMRepresentation(stype);
                    auto llvmPtrType = PointerType::getUnqual(llvmType);
                    auto newPhi = builder.CreatePHI(llvmPtrType, phi->getNumIncomingValues());
                    errs() << "out of the old phi a new one is born" << *newPhi
                           << "\n";
                    phiNodesToPopulate.insert(phi);
                    replacementMapping[phi] = newPhi;
                }
            } else if (auto callIns = dyn_cast<CallInst>(&ins)) {
                auto callee = callIns->getCalledFunction();
                if (callee == nullptr)
                    continue;
                auto calleeName = callee->getName().str();

                if (!llvm::memoir::isMemOIRCall(*callee)) {
                    continue;
/*          if (clonedFunctionMap.find(callee) == clonedFunctionMap.end()) {
//            continue;
//          }
//          // this is a function call which passes or returns Object*s
//          // replace the arguments
//          std::vector<Value *> arguments;
//          for (auto &arg : callIns->args()) {
//            auto val = arg.get();
//            if (replacementMapping.find(val) != replacementMapping.end()) {
//              arguments.push_back(replacementMapping[val]);
//            } else {
//              arguments.push_back(val);
//            }
//          }
//          auto new_callins =
//              builder.CreateCall(clonedFunctionMap[callee], arguments);
//          // if the return type isn't Object*, then we assume it is an
//          //intrinsic
//          // and its uses must be replaced
//          if (callIns->getType() != this->object_star) {
//            assert(new_callins->getType() == callIns->getType());
//            ins.replaceAllUsesWith(new_callins);
//          }
//          replacementMapping[callIns] = new_callins;*/
                } else {

                    auto calleeCategory = llvm::memoir::getMemOIREnum(*callee);
                    switch (calleeCategory) {
                        case ALLOCATE_TENSOR: {
                            auto &accAna = memoir::AllocationAnalysis::get(M);
                            auto allocSum = accAna.getAllocationSummary(*callIns);
                            assert(allocSum);
                            errs() << "here0\n";
                            auto tensorAllocSum = static_cast<TensorAllocationSummary *>(allocSum);
                            auto numdim = tensorAllocSum->getNumberOfDimensions();
                            auto &eletype = tensorAllocSum->getElementType();
                            auto &tensorType = static_cast<TensorTypeSummary &>(allocSum->getType());
                            Type *llvmType;
                            llvmType = nativeTypeConverter->getLLVMRepresentation(eletype);
                            errs() << "here1\n";
                            auto llvmTypeSize = llvm::ConstantInt::get(
                                    int64Ty,
                                    M.getDataLayout().getTypeAllocSize(llvmType));

                            Value *finalSize;
                            std::vector<int64_t> sizes;
                            auto isstatictensor = isStaticTensor(tensorAllocSum, sizes);
                            if (isstatictensor) {
                                assert(sizes.size() == numdim);
                                uint64_t size = 1;
                                for (uint64_t i = 0; i < numdim; ++i) {
                                    errs() << "dimention " << i << "has size " << sizes[i] << "\n";
                                    size = size * sizes[i];
                                }
                                auto finalCountVal = llvm::ConstantInt::get(int64Ty, size);
                                finalSize = builder.CreateMul(finalCountVal, llvmTypeSize);
                            } else {
                                finalSize = llvm::ConstantInt::get(int64Ty, 1);
                                for (unsigned long long i = 0; i < numdim; ++i) {
                                    finalSize = builder.CreateMul(finalSize, tensorAllocSum->getLengthOfDimension(i));
                                }
                                auto int64Size = llvm::ConstantInt::get(int64Ty,
                                                                        M.getDataLayout().getTypeAllocSize(int64Ty));

                                auto headerSize = builder.CreateMul(llvm::ConstantInt::get(int64Ty, numdim),
                                                                    int64Size);
                                finalSize = builder.CreateAdd(finalSize, headerSize);
                            }
                            errs() << "here2\n";
                            std::vector<Value *> arguments{finalSize};
                            auto newMallocCall = builder.CreateCall(mallocf, arguments);
                            auto llvmTypeTensor = nativeTypeConverter->getLLVMRepresentation(tensorType);
                            auto bcInst = builder.CreateBitCast(newMallocCall, llvmTypeTensor);
                            replacementMapping[callIns] = bcInst;

                            errs() << "here3\n";
                            if (!isstatictensor) {
                                for (unsigned long long i = 0; i < numdim; ++i) {
                                    Value *indexList[1] = {ConstantInt::get(int64Ty, i)};
                                    auto gep = builder.CreateGEP(PointerType::getUnqual(int64Ty), newMallocCall,
                                                                 indexList);
                                    auto storeIns = builder.CreateStore(tensorAllocSum->getLengthOfDimension(i), gep);
                                }
                            }

                            errs() << "here4\n";
                            break;
                        }
                        case ALLOCATE_STRUCT: {
                            // create malloc based on the object's LLVMRepresentation ;
                            //bitcast
                            // to a ptr to LLVMRepresentation
                            if (allocaBuildObj.find(callIns) != allocaBuildObj.end()) {
                                continue;
                            }
                            std::set<PHINode *> visited;
                            auto &accAna = memoir::AllocationAnalysis::get(M);
                            auto allocSum = accAna.getAllocationSummary(*callIns);
                            assert(allocSum);;
                            auto &typ = allocSum->getType();
//                            typ.
                            auto llvmType = nativeTypeConverter->getLLVMRepresentation(typ);
                            auto llvmTypeSize = llvm::ConstantInt::get(
                                    int64Ty,
                                    M.getDataLayout().getTypeAllocSize(llvmType));
                            std::vector<Value *> arguments{llvmTypeSize};
                            auto newMallocCall = builder.CreateCall(mallocf, arguments);

                            auto bc_inst =
                                    builder.CreateBitCast(newMallocCall,
                                                          PointerType::getUnqual(llvmType));

                            replacementMapping[callIns] = bc_inst;
                            break;
                        }
                        case WRITE_INT8:
                        case WRITE_INT16:
                        case WRITE_INT32:
                        case WRITE_INT64:
                        case WRITE_UINT8:
                        case WRITE_UINT16:
                        case WRITE_UINT32:
                        case WRITE_UINT64:
                        case WRITE_FLOAT:
                        case WRITE_INTEGER:
                        case WRITE_DOUBLE:
                        case WRITE_REFERENCE: {
                            auto[gep, fieldType] = GetGEPAndFieldSummary(callIns, builder, replacementMapping);
                            switch (fieldType.getCode()) {
                                case FloatTy:
                                case DoubleTy:
                                case IntegerTy: {
                                    auto storeInst =
                                            builder.CreateStore(callIns->getArgOperand(1), gep);
                                    errs() << "the store ins for writing reference is " << *storeInst<< "\n";
                                    replacementMapping[callIns] = storeInst;
                                    break;
                                }
                                case ReferenceTy: {

                                    auto new_val = callIns->getArgOperand(1);
                                    assert(replacementMapping.find(new_val) != replacementMapping.end());
                                    auto replPtr = replacementMapping[new_val];
                                    auto bc_inst = builder.CreateBitCast(replPtr, i8StarTy);
                                    auto storeInst = builder.CreateStore(bc_inst, gep);
                                    replacementMapping[callIns] = storeInst;
                                    break;
                                }
                                case StructTy:
                                case TensorTy:
                                    assert(false && "can't write a tensor and struct");
                            }
                            break;
                        }
                        case READ_INT8:
                        case READ_INT16:
                        case READ_INT32:
                        case READ_INT64:
                        case READ_UINT8:
                        case READ_UINT16:
                        case READ_UINT32:
                        case READ_UINT64:
                        case READ_FLOAT:
                        case READ_DOUBLE:
                        case READ_STRUCT:
                        case READ_TENSOR:
                        case READ_INTEGER:
                        case READ_REFERENCE: {
                            auto[gep, fieldType] = GetGEPAndFieldSummary(callIns, builder, replacementMapping);
                            switch (fieldType.getCode()) {

                                case FloatTy:
                                case DoubleTy:
                                case IntegerTy: {
                                    auto targetType = nativeTypeConverter->getLLVMRepresentation(fieldType);
                                    auto loadInst =
                                            builder.CreateLoad(targetType, gep, "baseload");
                                    replacementMapping[callIns] = loadInst;
                                    ins.replaceAllUsesWith(loadInst);
                                    break;
                                }
                                case ReferenceTy: {
                                    auto loadInst = builder.CreateLoad(i8StarTy, gep,
                                                                       "refload");
                                    // fetch the Type*, which should be a PointerTy/APointerType
                                    auto &refPtr = static_cast<ReferenceTypeSummary &>( fieldType);
                                    // the pointsTo must be an ObjectType, which we can use to get
                                    //the
                                    // target type for bitcast
                                    auto &refTy = refPtr.getReferencedType();
                                    auto llvmtype = nativeTypeConverter->getLLVMRepresentation(refTy);
                                    auto bc_inst =
                                            builder.CreateBitCast(loadInst,
                                                                  PointerType::getUnqual(llvmtype));
                                    replacementMapping[callIns] = bc_inst;
                                    break;
                                }
                                case StructTy:
                                case TensorTy:
                                    replacementMapping[callIns] = gep;
                                    break;
                            }
                            break;

                            // errs() << "out of the write gep is born" << *gep <<"\n";
                            // errs() << "from the readuint64 we have a load" << *loadInst
                            // <<"\n";
                            break;
                        }
                        case DELETE_OBJECT: {

                            auto arg0 = callIns->getArgOperand(0);
                            if (allocaBuildObj.find(dyn_cast<CallInst>(arg0))
                                != allocaBuildObj.end()) {
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
                        case DEFINE_STRUCT_TYPE:
                        case STRUCT_TYPE:
                        case TENSOR_TYPE:
                        case REFERENCE_TYPE:
                        case INTEGER_TYPE:
                        case UINT64_TYPE:
                        case UINT32_TYPE:
                        case UINT16_TYPE:
                        case UINT8_TYPE:
                        case INT64_TYPE:
                        case INT32_TYPE:
                        case INT16_TYPE:
                        case INT8_TYPE:
                        case FLOAT_TYPE:
                        case DOUBLE_TYPE:
                        case GET_STRUCT_FIELD:
                        case GET_TENSOR_ELEMENT:
                        case ASSERT_TYPE:
                        case SET_RETURN_TYPE:
                        case NONE:
                            break;
                    } // endof switch
                }
            } else if (auto bitcast = dyn_cast<BitCastInst>(&ins)) {
                auto casted = bitcast->getOperand(0);
                if (replacementMapping.find(casted) != replacementMapping.end()) {
                    replacementMapping[bitcast] = replacementMapping[casted];
                    errs() << *bitcast << "now is linked to the same entry as " << *casted << "which is "
                           << *(replacementMapping[casted]) << "\n";
                }
            } else if (auto icmp = dyn_cast<ICmpInst>(&ins)) {
                if (icmp->getOperand(0)->getType() != object_star) {
                    continue;
                }
                Value *not_the_null_one =
                        dyn_cast<ConstantPointerNull>(icmp->getOperand(0))
                        ? icmp->getOperand(1)
                        : icmp->getOperand(0);
                if (replacementMapping.find(not_the_null_one)
                    == replacementMapping.end()) {
//          errs()
//              << "Comparing object* but fail to find it in replacement
//              mapping";
                    assert(false);
                }
                auto newType = replacementMapping[not_the_null_one]->getType();
                assert(isa<PointerType>(newType));
                auto pointerNewType = dyn_cast<PointerType>(newType);
                Value *new_left;
                auto curLeft = icmp->getOperand(0);
                if (isa<ConstantPointerNull>(curLeft)) {
                    new_left = ConstantPointerNull::get(pointerNewType);
                } else if (replacementMapping.find(curLeft) ==
                           replacementMapping.end()) {
                    errs() << "can't find " << *curLeft << "in replacement mapping";
                    assert(false);
                } else {
                    new_left = replacementMapping[curLeft];
                }
                Value *new_right;
                auto curRight = icmp->getOperand(1);
                if (isa<ConstantPointerNull>(curRight)) {
                    new_right = ConstantPointerNull::get(pointerNewType);
                } else if (replacementMapping.find(curRight)
                           == replacementMapping.end()) {
                    errs() << "can't find " << *curRight << "in replacement mapping";
                    assert(false);
                } else {
                    new_right = replacementMapping[curRight];
                }
                errs() << "the left operand is " << *new_left << "\n";
                errs() << "the right operand is " << *new_right << "\n";
                auto newIcmp =
                        builder.CreateICmp(icmp->getPredicate(), new_left, new_right);
                replacementMapping[icmp] = newIcmp;
                icmp->replaceAllUsesWith(newIcmp);
            } else if (auto retIns = dyn_cast<ReturnInst>(&ins)) {
                // replace returned value, if necessary
                auto r_val = retIns->getReturnValue();
                if (replacementMapping.find(r_val) != replacementMapping.end()) {
                    auto new_ret = builder.CreateRet(replacementMapping[r_val]);
                    replacementMapping[retIns] = new_ret;
                }
            }
        }
        // transform the children in dominator-order
        auto node = DT.getNode(bb);
        for (auto child: node->getChildren()) {
            auto dominated = child->getBlock();
            BasicBlockTransformer(DT,
                                  dominated,
                                  replacementMapping,
                                  phiNodesToPopulate,
                                  allocaBuildObj);
        }
    } // endof BasicBlockTransformer



/*
//    Value *ObjectLowering::CreateGEPFromFieldWrapper(
//            FieldWrapper *fieldWrapper,
//            IRBuilder<> &builder,
//            std::map<Value *, Value *> &replacementMapping) {
////        return nullptr;
//          auto int32Ty = llvm::Type::getInt32Ty(M.getContext());
//        errs() << "CreateGEPFromFieldWrapper\n";
//        errs() << "\tField Wrapper Base " << *(fieldWrapper->baseObjPtr) << "\n";
//        errs() << "\tField Wrapper obj type " << fieldWrapper->objectType->toString()
//        << "\n"; errs() << "\tField Wrapper index " << fieldWrapper->fieldIndex <<
//        "\n";
//          auto llvmType = fieldWrapper->objectType->getLLVMRepresentation(M);
//          std::vector<Value *> indices = {
//            llvm::ConstantInt::get(int32Ty, 0),
//            llvm::ConstantInt::get(int32Ty, fieldWrapper->fieldIndex)
//          };
//          if (replacementMapping.find(fieldWrapper->baseObjPtr)
//              == replacementMapping.end()) {
//            errs() << "unable to find the base pointer " <<
//            *fieldWrapper->baseObjPtr
//                   << "\n";
//            assert(false);
//          }
//          auto llvmPtrType = PointerType::getUnqual(llvmType);
//          auto gep =
//          builder.CreateGEP(replacementMapping.at(fieldWrapper->baseObjPtr),
//                                       indices);
//          return gep;
//    }
*/

    void ObjectLowering::findInstsToDelete(Value *i, std::set<Value *> &toDelete) {
        for (auto u: i->users()) {
            if (toDelete.find(u) != toDelete.end())
                continue;
            toDelete.insert(u);
            findInstsToDelete(u, toDelete);
        }
    }

    std::pair<Value *, memoir::TypeSummary &>
    ObjectLowering::GetGEPAndFieldSummary(CallInst *callIns, IRBuilder<> &builder,
                                          std::map<Value *, Value *> &replacementMapping) {
        auto &ctxt = M.getContext();
        auto int32Ty = llvm::Type::getInt32Ty(ctxt);
        auto int64Ty = llvm::Type::getInt64Ty(ctxt);
        auto &accAna = memoir::AccessAnalysis::get(M);
        errs() << "Obtaining access analysis for the call ins " << *callIns << "\n\n";
        auto accSum = accAna.getAccessSummary(*callIns);
        auto getFirstFieldSumMustRead = [&](AccessSummary *accSum) -> FieldSummary & {
            return static_cast<MustReadSummary *>(accSum)->getField();
        };
        auto getFirstFieldSumMayRead = [&](AccessSummary *accSum) -> FieldSummary & {
            return *(*(static_cast<MayReadSummary *>(accSum)->begin()));
        };
        auto getFirstFieldSumMustWrite = [&](AccessSummary *accSum) -> FieldSummary & {
            return static_cast<MustWriteSummary *>(accSum)->getField();
        };
        auto getFirstFieldSumMayWrite = [&](AccessSummary *accSum) -> FieldSummary & {
            return *(*(static_cast<MayWriteSummary *>(accSum)->begin()));
        };
        auto &field = accSum->isMay() ?
                      (accSum->isRead() ? getFirstFieldSumMayRead(accSum) : getFirstFieldSumMayWrite(accSum)) :
                      (accSum->isRead() ? getFirstFieldSumMustRead(accSum) : getFirstFieldSumMustWrite(accSum));
        auto &fieldType = accSum->getType();
        auto llvmType = nativeTypeConverter->getLLVMRepresentation(fieldType);
        auto fieldCallIns = dyn_cast<CallInst>(callIns->getArgOperand(0));
        auto baseObj = fieldCallIns->getArgOperand(0);
        Value *gep;
        assert(replacementMapping.find(baseObj) != replacementMapping.end());
        auto replacedBaseObj = replacementMapping.at(baseObj);
        switch (field.getAllocation().getCode()) {
            case STRUCT: {
                auto objField = static_cast<StructFieldSummary &>(field);
                auto fieldIndex = objField.getIndex();
                Value *indices[2] = {llvm::ConstantInt::get(int64Ty, 0), llvm::ConstantInt::get(int64Ty, fieldIndex)};
                gep = builder.CreateGEP(replacedBaseObj,
                                        indices);
                break;
            }
            case TENSOR: {
                auto tensorField = static_cast<TensorElementSummary &>(field);
                auto &allocAna = memoir::AllocationAnalysis::get(M);
                auto allocsums = allocAna.getAllocationSummaries(*baseObj);
                std::vector<int64_t > constantSizes;
                bool isstatic = false;
                for (auto &allocsum : allocsums)
                {
                    auto tas = static_cast<TensorAllocationSummary *>(allocsum);
                    isstatic = isStaticTensor(tas,constantSizes);
                    if (isstatic)
                    {
                        break;
                    }
                }
                errs() << "here5\n";
                auto tensorType = static_cast<TensorTypeSummary &>(field.pointsTo().getType());
//                auto tensorType = static_cast<TensorTypeSummary &>(field.getType());
                errs() << "here6\n";
                auto ndim = tensorType.getNumDimensions();
                errs() << "actual dims? " << ndim << "\n";
                Value *sizes[ndim];
                if (isstatic) {
                    if(tensorType.isStaticLength()) {
                        for (uint64_t i = 0; i < ndim; ++i) {
                            sizes[i] = llvm::ConstantInt::get(int64Ty, tensorType.getLengthOfDimension(i));
                        }
                    }else
                    {
                        for (uint64_t i = 0; i < ndim; ++i) {
                            sizes[i] = llvm::ConstantInt::get(int64Ty, constantSizes[i]);
                        }
                    }
                    replacedBaseObj = builder.CreateBitCast(replacedBaseObj, PointerType::getUnqual(llvmType));
                    errs() << "now the base pointer is " << *replacedBaseObj << "\n";
                } else {
                    for (unsigned long long i = 0; i < ndim; ++i) {
                        Value *indexList[1] = {ConstantInt::get(int64Ty, i)};
                        auto sizeGEP = builder.CreateGEP(PointerType::getUnqual(int64Ty), replacedBaseObj, indexList);
                        sizes[i] = builder.CreateLoad(sizeGEP);
                    }
                    Value *indexList[1] = {ConstantInt::get(int64Ty, ndim)};
                    auto skipMetaDataGEP = builder.CreateGEP(PointerType::getUnqual(int64Ty), replacedBaseObj,
                                                             indexList);
                    replacedBaseObj =
                            builder.CreateBitCast(skipMetaDataGEP,
                                                  PointerType::getUnqual(llvmType));
                }

                Value *multiCumSizes[ndim];
                multiCumSizes[ndim - 1] = ConstantInt::get(int64Ty, 1);
                errs() << "dimention = " << ((signed long long )ndim) << "\n";
                for (signed long long i = ((signed long long )ndim) - 2; i >= 0; --i) {
                    errs() << "dimention " << i << " uses" << *multiCumSizes[i + 1] << "and " << *sizes[i + 1] << " \n";
                    multiCumSizes[i] = builder.CreateMul(multiCumSizes[i + 1], sizes[i + 1]);
                }
                errs() << "here7\n";
                Value *size = ConstantInt::get(int64Ty, 0);
                for (unsigned long long dim = 0; dim < ndim; ++dim) {
                    errs() << "Dimension size" << dim << "represented by " <<   tensorField.getIndex(dim) << "\n";
                    auto skipsInDim = builder.CreateMul(multiCumSizes[dim], &tensorField.getIndex(dim));
                    size = builder.CreateAdd(size, skipsInDim);
                }
                Value *indices[1] = {size};
                gep = builder.CreateGEP(replacedBaseObj,
                                        indices);
                break;
            }
        }
        std::pair<Value *, TypeSummary &> pair(gep, fieldType);
        return pair;
    }
} // namespace object_lowering
