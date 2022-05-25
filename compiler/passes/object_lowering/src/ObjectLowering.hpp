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

  // collect all GlobalVals which are Type*
  std::vector<GlobalValue *> typeDefs;

  std::map<Function*, Function*> clonedFunctionMap;
  std::map<Function*, map<Argument *, Argument *>> functionArgumentMaps;


public:
  ObjectLowering(Module &M, Noelle *noelle, ModulePass* mp);
  
  // ==================== ANALYSIS ====================

  void analyze();

  void cacheTypes(); // analyze the global values for type*

  // handle object passing and returning
  void inferArgTypes(llvm::Function* f, vector<Type*> *arg_vector); // build a new list of argument types
  ObjectType* inferReturnType(llvm::Function* f);

  // excluding objectIR functions from analysis
  bool shouldAnalyzeFunction(llvm::Function& F);

  // ======================== STACK VS HEAP =====================

  DataFlowResult *dataflow(Function *f, std::set<CallInst *> &buildObjs);

  // ==================== TRANSFORMATION ====================

  void transform();

  void FunctionTransform(Function* func);
  
  void BasicBlockTransformer(DominatorTree &DT, BasicBlock *bb,
                             std::map<Value*, Value*> &replacementMapping,
                             std::set<PHINode*>& phiNodesToPopulate,
                             std::set<CallInst*>& allocaBuildObj);

  Value* CreateGEPFromFieldWrapper(FieldWrapper *wrapper, IRBuilder<> &builder, std::map<Value*, Value*> &replacementMapping);

  // recursively add users of `i` to `toDelete`
  void findInstsToDelete(Value* i, std::set<Value*> &toDelete);
  };

} // namespace object_lowering
