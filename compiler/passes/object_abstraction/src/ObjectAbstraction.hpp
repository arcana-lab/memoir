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

class TypeNode {
private:
  CallInst *definition;
  int numFields;
  std::vector<CallInst *> fields;

public:
  TypeNode(CallInst *inst);
  void printTypeInfo();
};

class ObjectNode {
  // TODO
};

class FieldNode {
  // TODO
};

class ObjectAbstraction {
private:
  Module &M;

  Noelle *noelle;

  PDG *pdg;

  std::unordered_set<CallInst *> callsToObjectIR;

  std::unordered_set<TypeNode *> types; // All definition of object types

public:
  ObjectAbstraction(Module &M, Noelle *noelle, PDG *pdg);
  
  void analyze();

  void transform();
};

} // namespace object_abstraction
