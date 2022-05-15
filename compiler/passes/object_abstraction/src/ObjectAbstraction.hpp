#pragma once

#include "noelle/core/Noelle.hpp"

#include "Utils.hpp"

/*
 * Pass to build object representations at object-IR level
 *
 * Author: Yian Su
 * Created: April 30, 2022
 */

namespace object_abstraction {

class TypeNode;
class FieldNode;
class ObjectNode;

class TypeNode {
private:
  CallInst *definition;
  int numFields;
  std::vector<CallInst *> fieldTypes;

public:
  ~TypeNode();
  TypeNode(CallInst *inst);
  inline int getNumFields() { return this->numFields; };
  inline CallInst * getFieldType(int fieldNum) { return this->fieldTypes[fieldNum]; };
  void printTypeInfo(string prefix);
};

class FieldNode {
private:
  ObjectNode *objectBelongsTo;
  CallInst *fieldType;
  std::unordered_set<CallInst *> fieldReads;
  std::unordered_set<CallInst *> fieldWrites;

public:
  ~FieldNode();
  FieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType);
  inline void addFieldRead(CallInst *fieldRead) { this->fieldReads.insert(fieldRead); };
  inline void addFieldWrite(CallInst *fieldWrite) { this->fieldWrites.insert(fieldWrite); };
  void printFieldInfo(string prefix);
};

class ObjectNode {
private:
  CallInst *allocation;
  TypeNode *manifest;
  std::unordered_map<CallInst *, FieldNode *> fieldAccessorMap;
  std::vector<FieldNode *> fields;
  CallInst *deletion;

public:
  ~ObjectNode();
  ObjectNode(CallInst *inst, TypeNode *typeNode);
  void addFieldAccessor(CallInst *fieldAccessor);
  inline void addFieldRead(CallInst *fieldAccessor, CallInst *fieldRead) { this->fieldAccessorMap[fieldAccessor]->addFieldRead(fieldRead); };
  inline void addFieldWrite(CallInst *fieldAccessor, CallInst *fieldWrite) { this->fieldAccessorMap[fieldAccessor]->addFieldWrite(fieldWrite); };
  inline void setObjectDeletion(CallInst *deletion) { this->deletion = deletion; };
  void printObjectInfo(string prefix);
};

class ObjectAbstraction {
private:
  Module &M;

  Noelle *noelle;

  PDG *pdg;

  std::unordered_set<CallInst *> callsToObjectIR;
  std::unordered_set<TypeNode *> types; // All definition of object 
  std::unordered_map<CallInst *, TypeNode *> callToTypeNodeMap; // Type definition to created TypeNode
  std::unordered_set<ObjectNode *> objects; // All allocation of objects
  std::unordered_map<CallInst *, ObjectNode *> callToObjectNodeMap; // Object allocation to created ObjectNode

public:
  ObjectAbstraction(Module &M, Noelle *noelle, PDG *pdg);
  
  void analyze();

  void transform();
};

} // namespace object_abstraction
