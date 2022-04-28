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

  //std::unordered_set<CallInst *> callsToObjectIR;
  std::unordered_set<CallInst *> buildObjects;
  std::unordered_set<CallInst *> reads;
  std::unordered_set<CallInst *> writes;
  std::map<CallInst*, ObjectWrapper*> buildObjMap;
  std::map<CallInst*, FieldWrapper*> readWriteFieldMap;
  std::set<PHINode*> visitedPhiNodesGlobal;
  std::set<Function*> functionsToProcess;

public:
  ObjectLowering(Module &M, Noelle *noelle, ModulePass* mp);
  
  void analyze();

  void transform();

  ObjectWrapper* parseObjectWrapperInstruction(CallInst* i, std::set<PHINode*> &visited);

  void parseType(Value* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);

  void parseTypeStoreInst(StoreInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);

  void parseTypeLoadInst(LoadInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);

  void parseTypeAllocaInst(AllocaInst* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);

  void parseTypeGlobalValue(GlobalValue* ins, const std::function<void(CallInst*)>&, std::set<PHINode*> &visited);

  AnalysisType* parseTypeCallInst(CallInst *ins, std::set<PHINode*> &visited);

  FieldWrapper* parseFieldWrapperIns(CallInst* i, std::set<PHINode*> &visited);


//  void parseTypeCallInst(CallInst* ins);


  /*
   * parseType(%0 = load %typety)
   * Calls
   * parseType(%typety = alloca .....)
   * Calls
   * parseType(%call = getObjectType(%x, %y))
       * Calls
       * parseType(%x) and parseType(%y)
       * parseType(%x = getint64) -> AnalysisType(int64)
       * parseType(%y = getint64) -> AnalysisType(int64)
   * parseType(%call = getObjectType(%x, %y))
   * ObjectType(int64, int64)
   * r
   * base case:
   *
   *
   *
   * if ins is a load from %typety
   *    go to the uses of typety find store
   *    value of store is going to be a callinst to get object type
   *    alloca for type*
   *    walk back to store untill we find call inst
   *    foreach parameters:
   *
   *
   *
   */



        void BasicBlockTransformer(DominatorTree &DT, BasicBlock *bb);
    };

} // namespace object_lowering
