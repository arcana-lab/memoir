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

  //std::unordered_set<CallInst *> callsToObjectIR;
  std::unordered_set<CallInst *> buildObjects;
  std::unordered_set<CallInst *> reads;
  std::unordered_set<CallInst *> writes;
  std::map<CallInst*, ObjectWrapper*> buildObjMap;
  std::map<CallInst*, FieldWrapper*> readWriteFieldMap;
  std::set<PHINode*> visitedPhiNodes;

public:
  ObjectLowering(Module &M, Noelle *noelle);
  
  void analyze();

  void transform();

  ObjectWrapper* parseObjectWrapperInstruction(CallInst* i);

  void parseType(Value* ins, std::function<void(CallInst*)>);

  void parseTypeStoreInst(StoreInst* ins, std::function<void(CallInst*)>);

  void parseTypeLoadInst(LoadInst* ins, std::function<void(CallInst*)>);

  void parseTypeAllocaInst(AllocaInst* ins, std::function<void(CallInst*)>);

  void parseTypeGlobalValue(GlobalValue* ins, std::function<void(CallInst*)>);

  AnalysisType* parseTypeCallInst(CallInst *ins);

  FieldWrapper* parseFieldWrapperIns(CallInst* i);


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



};

} // namespace object_lowering
