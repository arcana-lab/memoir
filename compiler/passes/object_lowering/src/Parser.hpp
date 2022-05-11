#pragma once

#include "noelle/core/Noelle.hpp"
#include "Utils.hpp"
#include "types.hpp"

namespace object_lowering {

class Parser {
private:
  Module &M;
  Noelle *noelle;
  ModulePass* mp;
  
public:
  Parser(Module &M, Noelle *noelle, ModulePass* mp);

  // Caches
  std::map<Instruction*, AnalysisType*> analysisTypeMap; // any CallInst -> type
  std::map<Value*, ObjectWrapper*> buildObjMap;
  
  // the CallInst must be an getObjectType, getPtrType, getUInt64, etc to reconstruct the Type*
  AnalysisType* parseTypeCallInst(CallInst *ins, std::set<PHINode*> &visited);
  std::string fetchString(Value* ins);

  // this function wraps over the second one ...
  // used by BBtransform/phi and parseFieldWrapperIns // REFACTOR: why is this Value*?
  ObjectWrapper* parseObjectWrapperChain(Value *i, set<PHINode *> &visited); 
  // create the ObjectWrapper* from the @buildObject CallInst ; do caching w/ buildObjMap
  ObjectWrapper* parseObjectWrapperInstruction(CallInst* i, std::set<PHINode*> &visited);

  // create the fieldWrapper from @getObjectField CallInst // REFACTOR: maybe we should cache these too?
  FieldWrapper* parseFieldWrapperIns(CallInst* i, std::set<PHINode*> &visited);

  FieldWrapper* parseFieldWrapperChain(Value* i, std::set<PHINode*> &visited);

  // dispatch to the functions below
  void parseType(Value* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
  void parseTypeStoreInst(StoreInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
  void parseTypeLoadInst(LoadInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
  void parseTypeAllocaInst(AllocaInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
  void parseTypeGlobalValue(GlobalValue* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);
};

} // namespace object_lowering
