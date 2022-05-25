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

/*
 * TypeNode
 */
class TypeNode {
private:
  CallInst *definition;
  std::vector<CallInst *> fieldTypes;

public:
  ~TypeNode();
  TypeNode(CallInst *callInst);
  TypeNode(CallInst *callInst, CallInst *innerTypeCallInst);
  inline CallInst * getDefinition() { return this->definition; };
  inline int getNumFields() { return this->fieldTypes.size(); };
  inline CallInst * getFieldType(int fieldNum) { return this->fieldTypes[fieldNum]; };
  void printTypeInfo(string prefix);
};

/*
 * FieldNode
 */
class FieldNode {
protected:
  ObjectNode *objectBelongsTo;
  CallInst *fieldType;
  std::unordered_set<CallInst *> fieldReads;
  std::unordered_set<CallInst *> fieldWrites;

public:
  ~FieldNode();
  FieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType);
  inline void addFieldWrite(CallInst *fieldWrite) { this->fieldWrites.insert(fieldWrite); };
  inline void addFieldRead(CallInst *fieldRead) { this->fieldReads.insert(fieldRead); };
  virtual void printFieldInfo(string prefix);
};

class SummaryFieldNode : public FieldNode {
private:
  TypeNode *innerObjectType;
  Value *size;

public:
  ~SummaryFieldNode();
  SummaryFieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType, TypeNode *typeNode, Value *size);
  void printFieldInfo(string prefix) override;
};

/*
 * ObjectNode
 */
class ObjectNode {
private:
  CallInst *allocation;
  TypeNode *manifest;
  std::vector<FieldNode *> fields;
  std::unordered_map<CallInst *, FieldNode *> fieldAccessMap;
  CallInst *deletion;

public:
  ~ObjectNode();
  ObjectNode(CallInst *callInst, TypeNode *typeNode);
  ObjectNode(CallInst *callInst, TypeNode *arrayTypeNode, TypeNode *innerTypeNode);
  void addFieldAccess(CallInst *fieldAccess);
  inline void addFieldWrite(CallInst *fieldAccess, CallInst *fieldWrite) { this->fieldAccessMap[fieldAccess]->addFieldWrite(fieldWrite); };
  inline void addFieldRead(CallInst *fieldAccess, CallInst *fieldRead) { this->fieldAccessMap[fieldAccess]->addFieldRead(fieldRead); };
  inline void setObjectDeletion(CallInst *deletion) { this->deletion = deletion; };
  virtual void printObjectInfo(string prefix);
};

class ObjectAbstraction {
private:
  Module &M;
  Noelle *noelle;
  PDG *pdg;

  std::unordered_set<TypeNode *> types;                             // All definition of object types
  std::unordered_map<CallInst *, TypeNode *> callToTypeNodeMap;     // Type definition to created TypeNode
  std::unordered_set<ObjectNode *> objects;                         // All allocation of objects
  std::unordered_map<CallInst *, ObjectNode *> callToObjectNodeMap; // Object allocation to created ObjectNode

public:
  ObjectAbstraction(Module &M, Noelle *noelle, PDG *pdg);

  void analyze();

  void transform();

  CallInst * retrieveTypeDefinition(Value *value);
};

} // namespace object_abstraction
