/*
 * Object abstractions at LLVM-IR level
 *
 * Author: Yian Su
 * Created: April 30, 2022
 */
#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "Utils.hpp"
#include "noelle/core/Noelle.hpp"

namespace object_abstraction {

class ObjectNode;

/*
 * TypeNode definition
 */
class TypeNode {
public:
  /*
   * Constructor for struct-like type definition
   */
  TypeNode(CallInst *callInst);

  /*
   * Constructor for array-like type definition
   */
  TypeNode(CallInst *callInst, CallInst *innerTypeCallInst);

  /*
   * Get definition CallInst for type
   */
  inline CallInst *getDefinition() {
    return this->definition;
  };

  /*
   * Return the number of type fields
   */
  inline int getNumFields() {
    return this->fieldTypes.size();
  };

  /*
   * Get definition CallInst of type field at index
   */
  inline CallInst *getFieldTypeAtIndex(int fieldIndex) {
    return this->fieldTypes[fieldIndex];
  };

  void printTypeInfo(string prefix = "");
  ~TypeNode();

private:
  CallInst *definition;
  std::vector<CallInst *> fieldTypes;
};

/*
 * FieldNode definition
 */
class FieldNode {
public:
  /*
   * Constructor for struct object field
   */
  FieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType);

  /*
   * Record a write to the field
   */
  inline void addFieldWrite(CallInst *fieldWrite) {
    this->fieldWrites.insert(fieldWrite);
  };

  /*
   * Record a read to the field
   */
  inline void addFieldRead(CallInst *fieldRead) {
    this->fieldReads.insert(fieldRead);
  };

  virtual void printFieldInfo(string prefix = "");
  ~FieldNode();

protected:
  ObjectNode *objectBelongsTo;
  CallInst *fieldType;
  std::unordered_set<CallInst *> fieldReads;
  std::unordered_set<CallInst *> fieldWrites;
};

/*
 * SummaryFieldNode definition
 */
class SummaryFieldNode : public FieldNode {
public:
  /*
   * Constructor for array summary field
   */
  SummaryFieldNode(ObjectNode *objectBelongsTo,
                   CallInst *fieldType,
                   TypeNode *typeNode,
                   Value *size);

  void printFieldInfo(string prefix = "") override;
  ~SummaryFieldNode();

private:
  TypeNode *innerObjectType;
  Value *size;
};

/*
 * ObjectNode definition
 */
class ObjectNode {
public:
  /*
   * Constructor for struct-like object
   */
  ObjectNode(CallInst *callInst, TypeNode *typeNode);

  /*
   * Constructor for array-like object
   */
  ObjectNode(CallInst *callInst,
             TypeNode *arrayTypeNode,
             TypeNode *innerTypeNode);

  /*
   * Record field access at object level
   * Tracking these access maybe unnecessary, leave it here for now to see can
   * we justify their usage
   */
  void addFieldAccess(CallInst *fieldAccess);

  /*
   * Record field write inside the object
   */
  inline void addFieldWrite(CallInst *fieldAccess, CallInst *fieldWrite) {
    this->fieldAccessMap[fieldAccess]->addFieldWrite(fieldWrite);
  };

  /*
   * Record field read inside the object
   */
  inline void addFieldRead(CallInst *fieldAccess, CallInst *fieldRead) {
    this->fieldAccessMap[fieldAccess]->addFieldRead(fieldRead);
  };

  /*
   * Set object delection
   */
  inline void setObjectDeletion(CallInst *deletion) {
    this->deletions.insert(deletion);
  };

  void printObjectInfo(string prefix = "");
  ~ObjectNode();

private:
  CallInst *allocation;
  TypeNode *manifest;
  uint64_t level;
  std::vector<FieldNode *> fields;
  std::unordered_map<CallInst *, FieldNode *> fieldAccessMap;
  std::unordered_set<CallInst *> deletions;
};

/*
 * ObjectAbstraction definition
 */
class ObjectAbstraction {
public:
  /*
   * Constructor for ObjectAbstraction
   */
  ObjectAbstraction(Module &M, Noelle *noelle);

  /*
   * Entry function to construct object-ir abstractions
   */
  void contructObjectAbstractions();

  /*
   * Return one level up of the type definition given the loaded value
   */
  static CallInst *retrieveTypeDefinition(Value *value);

  ~ObjectAbstraction();

private:
  Module &M;
  Noelle *noelle;

  std::unordered_set<TypeNode *> types; // All definition of types
  std::unordered_map<CallInst *, TypeNode *>
      callToTypeNodeMap;                    // Type definition to TypeNode
  std::unordered_set<ObjectNode *> objects; // All allocation of objects
  std::unordered_map<CallInst *, ObjectNode *>
      callToObjectNodeMap; // Object allocation to ObjectNode

  /*
   * Module pass to collect all global type definitions
   */
  void collectTypeDefinitions();

  /*
   * Module pass to collect all object allocations
   */
  void collectObjectAllocations();

  /*
   * Build objectNode recursively given definition of object type
   *
   * Example: the initialization of the below type definition will create two
   * ObjectNodes Object { Object { uint64 uint64
   *   }
   *   uint64
   * }
   */
  void allocateObjectNodeRecursively(CallInst *buildObjectCallInst,
                                     CallInst *getObjectTypeCallInst,
                                     uint64_t level);

  /*
   * Module pass to collect all object accesses and deallocations
   */
  void collectObjectAccessesAndDeletions();

  /*
   * Module pass to collect all field writes and reads
   */
  void collectFieldWritesAndReads();

  /*
   * Print out all types and objects collected
   */
  void printTypesAndObjectsInfo();
};

/*
 * ObjectAbstraction Module Pass
 */
class ObjectAbstractionPass : public ModulePass {
public:
  static char ID;

  ObjectAbstractionPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  /*
   * Fetch constructed ObjectAbstraction object
   */
  inline ObjectAbstraction *getObjectAbstraction() {
    return this->objectAbstraction;
  };

private:
  ObjectAbstraction *objectAbstraction;
};

} // namespace object_abstraction
