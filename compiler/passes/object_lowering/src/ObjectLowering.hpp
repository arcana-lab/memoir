#pragma once

#include "noelle/core/Noelle.hpp"
#include "Utils.hpp"
#include "types.hpp"
#include "Parser.hpp"
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
  Parser *parser;

  // llvm Type*s
  Type* object_star;
  Type* type_star;
  Type* type_star_star;

  std::unordered_set<CallInst *> buildObjects;
  std::unordered_set<CallInst *> reads;
  std::unordered_set<CallInst *> writes;
  
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

  void cacheTypes(); // analyze the global values for type*

  void inferArgTypes(llvm::Function* f, vector<Type*> *arg_vector); // build a new list of argument types
  Type* inferReturnType(llvm::Function* f);

  // proof of concept temp impl:
  void tmpPatchup(Function* oldF, Function* newF);
  void tmpDomTreeTraversal(DominatorTree &DT, BasicBlock *bb, map<Argument*, Argument*> *old_to_new);

  // ==================== TRANSFORMATION ====================

  void transform();
  
  void BasicBlockTransformer(DominatorTree &DT, BasicBlock *bb);

  Value* CreateGEPFromFieldWrapper(FieldWrapper *wrapper, IRBuilder<> &builder);

  // recursively add users of `i` to `toDelete`
  void findInstsToDelete(Value* i, std::set<Value*> &toDelete);
  };

} // namespace object_lowering
