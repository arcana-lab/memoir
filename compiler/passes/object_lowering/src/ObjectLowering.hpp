#pragma once

#include "noelle/core/Noelle.hpp"
#include "Utils.hpp"
#include "types.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"

/*
 * Pass to perform lowering from object-ir to LLVM IR
 *
 * Author: Tommy McMichen
 * Created: March 29, 2022
 */

namespace object_lowering {

class ObjectLowering {
private:
  Module &M;
  Noelle *noelle;
  ModulePass* mp;

  Type* llvmObjectType; // hacky way to get the represenation of Object* type in llvm

  std::unordered_set<CallInst *> buildObjects;
  std::unordered_set<CallInst *> reads; // todo we use this?
  std::unordered_set<CallInst *> writes;
  std::map<CallInst*, ObjectWrapper*> buildObjMap;
  std::map<CallInst*, FieldWrapper*> readWriteFieldMap;
  std::set<Function*> functionsToProcess;
  
  std::map<Value*, Value*> replacementMapping;
  std::set<PHINode*> phiNodesToPopulate;
  
  std::map<Instruction*, AnalysisType*> inst_to_a_type; // cache AnalysisTypes // TODO: what is this??

public:
  ObjectLowering(Module &M, Noelle *noelle, ModulePass* mp);
  
  // ==================== ANALYSIS ====================

  void analyze();

  AnalysisType* parseTypeCallInst(CallInst *ins, std::set<PHINode*> &visited);

  ObjectWrapper* parseObjectWrapperChain(Value *i, set<PHINode *> &visited); // this function wraps over the second one ...
  ObjectWrapper* parseObjectWrapperInstruction(CallInst* i, std::set<PHINode*> &visited); // TODO: why 2 of these?

  
  FieldWrapper* parseFieldWrapperIns(CallInst* i, std::set<PHINode*> &visited);

  void parseType(Value* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);

  void parseTypeStoreInst(StoreInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
  void parseTypeLoadInst(LoadInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
  void parseTypeAllocaInst(AllocaInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
  void parseTypeGlobalValue(GlobalValue* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);

  // ==================== TRANSFORMATION ====================

  void transform();
  
  void BasicBlockTransformer(DominatorTree &DT, BasicBlock *bb);

  Value* CreateGEPFromFieldWrapper(FieldWrapper *wrapper, IRBuilder<> &builder);

  void findInstsToDelete(Value* i, std::set<Value*> &toDelete);
  };

} // namespace object_lowering
