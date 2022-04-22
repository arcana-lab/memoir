#pragma once

#include "noelle/core/Noelle.hpp"

#include "Utils.hpp"

#include "types.hpp"

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
  std::unordered_set<CallInst *> readUINT64;
  std::unordered_set<CallInst *> writeUINT64;

public:
  ObjectLowering(Module &M, Noelle *noelle);
  
  void analyze();

  void transform();

  ObjectWrapper* parseObjectWrapperInstruction(CallInst* i);

  Type* parseType(Value* ins);

  Type* parseTypeCallInst(CallInst* ins);

  Type* parseTypeStoreInst(StoreInst* ins);

  Type* parseTypeLoadInst(LoadInst* ins);

  Type* parseTypeAllocaInst(AllocaInst* ins);

  Type* parseTypeGlobalValue(GlobalValue* ins);




  /*
   * parseType(%0 = load %typety)
   * Calls
   * parseType(%typety = alloca .....)
   * Calls
   * parseType(%call = getObjectType(%x, %y))
       * Calls
       * parseType(%x) and parseType(%y)
       * parseType(%x = getint64) -> Type(int64)
       * parseType(%y = getint64) -> Type(int64)
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
