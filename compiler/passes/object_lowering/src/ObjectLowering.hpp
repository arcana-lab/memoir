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
  std::map<Instruction*, AnalysisType*> analysisTypeMap;

  std::unordered_set<CallInst *> buildObjects;
  std::unordered_set<CallInst *> reads;
  std::unordered_set<CallInst *> writes;
  
  std::map<CallInst*, ObjectWrapper*> buildObjMap;
  std::map<CallInst*, FieldWrapper*> readWriteFieldMap;
  
  std::set<Function*> functionsToProcess;
  
  // TODO: these maps are cleared for every function transformed, but the others aren't
  // we should reconsider this when implementing the interprocedural pass
  std::map<Value*, Value*> replacementMapping;
  std::set<PHINode*> phiNodesToPopulate;

public:
  ObjectLowering(Module &M, Noelle *noelle, ModulePass* mp);
  
  // ==================== ANALYSIS ====================

  void analyze();

  // the CallInst must be an getObjectType, getPtrType, getUInt64, etc to reconstruct the Type*
  AnalysisType* parseTypeCallInst(CallInst *ins, std::set<PHINode*> &visited);

  // this function wraps over the second one ...
  // used by BBtransform/phi and parseFieldWrapperIns // REFACTOR: why is this Value*?
  ObjectWrapper* parseObjectWrapperChain(Value *i, set<PHINode *> &visited); 
  // create the ObjectWrapper* from the @buildObject CallInst ; do caching w/ buildObjMap
  ObjectWrapper* parseObjectWrapperInstruction(CallInst* i, std::set<PHINode*> &visited);

  // create the fieldWrapper from @getObjectField CallInst // REFACTOR: maybe we should cache these too?
  FieldWrapper* parseFieldWrapperIns(CallInst* i, std::set<PHINode*> &visited);

  // dispatch to the functions below
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
