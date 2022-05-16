#include "ObjectAbstraction.hpp"

using namespace object_abstraction;

/*
 * Constructor for both struct and array type definition
 */
TypeNode::TypeNode(CallInst *callInst)
  : definition(callInst) {
  for (int i = 1; i < callInst->getNumArgOperands(); i++) {
    if (auto fieldDefCallInst = dyn_cast<CallInst>(callInst->getArgOperand(i))) {
      this->fieldTypes.push_back(fieldDefCallInst);
    }
  }

  return;
}

void TypeNode::printTypeInfo(string prefix="") {
  errs() << prefix << "Type definition: " << *this->definition << "\n";
  errs() << prefix << "  Num of fields: " << this->fieldTypes.size() << "\n";
  for (auto fieldType : this->fieldTypes) {
    errs() << prefix << "    Field: " << *fieldType << "\n";
  }

  return;
}

/*
 * Constructor for struct field
 */
FieldNode::FieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType)
  : objectBelongsTo(objectBelongsTo),
    fieldType(fieldType) {

  return;
}

void FieldNode::printFieldInfo(string prefix="") {
  errs() << prefix << "Field type: " << *this->fieldType << "\n";
  errs() << prefix << "  Writes:\n";
  for (auto fieldWrite : this->fieldWrites) {
    errs() << prefix << "    " << *fieldWrite << "\n";
  }
  errs() << prefix << "  Reads:\n";
  for (auto fieldRead : this->fieldReads) {
    errs() << prefix << "    " << *fieldRead << "\n";
  }

  return;
}

/*
 * Constructor for struct field
 */
SummaryFieldNode::SummaryFieldNode(ObjectNode *objectBelongsTo, TypeNode *typeNode, Value *size) 
  : FieldNode(objectBelongsTo, nullptr),
    innerObjectType(typeNode),
    size(size) {

  return;
}

void SummaryFieldNode::printFieldInfo(string prefix) {
  errs() << prefix << "Inner object type:\n";
  this->innerObjectType->printTypeInfo(prefix + "  ");
  errs() << prefix << "Field size: " << *this->size << "\n";
  errs() << prefix << "Writes:\n";
  for (auto fieldWrite : this->fieldWrites) {
    errs() << prefix << "    " << *fieldWrite << "\n";
  }
  errs() << prefix << "Reads:\n";
  for (auto fieldRead : this->fieldReads) {
    errs() << prefix << "    " << *fieldRead << "\n";
  }
}

/*
 * Constructor for both struct object
 */
ObjectNode::ObjectNode(CallInst *callInst, TypeNode *typeNode)
  : allocation(callInst),
    manifest(typeNode) {
  for (int i = 0; i < typeNode->getNumFields(); i++) {
    FieldNode *fieldNode = new FieldNode(this, typeNode->getFieldType(i));
    this->fields.push_back(fieldNode);
  }

  return;
}

/*
 * Constructor for array object
 */
ObjectNode::ObjectNode(CallInst *callInst, TypeNode *arrayTypeNode, TypeNode *innerTypeNode) 
  : allocation(callInst),
    manifest(arrayTypeNode) {
  this->summaryField = new SummaryFieldNode(this, innerTypeNode, callInst->getArgOperand(1));

  return;
}

void ObjectNode::addFieldAccess(CallInst *fieldAccess) {
  if (this->manifest->getNumFields() > 0) {       // accessing struct-like object
    if (auto fieldIndexConstant = dyn_cast<ConstantInt>(fieldAccess->getArgOperand(1))) {
      this->fieldAccessMap[fieldAccess] = this->fields[fieldIndexConstant->getSExtValue()];
    }
  } else {                                        // accessing array-like object
    this->fieldAccessMap[fieldAccess] = this->summaryField;
  }

  return;
}

void ObjectNode::printObjectInfo(string prefix="") {
  errs() << prefix << "Object allocation: " << *this->allocation << "\n";
  errs() << prefix << "Manifest: " << "\n";
  this->manifest->printTypeInfo(prefix + "  ");
  errs() << prefix << "Object accesses: " << "\n";
  for (auto &pair : this->fieldAccessMap) {
    errs() << prefix << "  " << *pair.first << "\n";
  }
  if (this->manifest->getNumFields() > 0) {     // looking at struct-like object
    errs() << "Object fields:\n";
    for (auto fieldNode : this->fields) {
      fieldNode->printFieldInfo(prefix + "  ");
    }
  } else {                                      // looking at array-like object
    errs() << "Object summary field:\n";
    this->summaryField->printFieldInfo(prefix + "  ");
  }
  errs() << prefix << "Object deallocation: " << *this->deletion << "\n";

  return;
}

ObjectAbstraction::ObjectAbstraction(Module &M, Noelle *noelle, PDG *pdg)
  : M(M),
    noelle(noelle),
    pdg(pdg) {
  // Do initialization.
}

void ObjectAbstraction::analyze() {

  errs() << "Gathering all type definitions\n";
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto callInst = dyn_cast<CallInst>(&I)) {
          auto callee = callInst->getCalledFunction();
          if (!callee) {
            continue;
          }

          if (callee->getName() == ObjectIRToFunctionNames[OBJECT_TYPE] || callee->getName() == ObjectIRToFunctionNames[ARRAY_TYPE]) {
            TypeNode *typeNode = new TypeNode(callInst);
            this->types.insert(typeNode);
            this->callToTypeNodeMap[callInst] = typeNode;
          }
        }
      }
    }
  }

  errs() << "Gathering all object allocations\n";
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto callInst = dyn_cast<CallInst>(&I)) {
          auto callee = callInst->getCalledFunction();
          if (!callee) {
            continue;
          }

          if (callee->getName() == ObjectIRToFunctionNames[BUILD_OBJECT]) {
            if (auto typeDefCallInst = this->retrieveTypeDefinition(callInst->getArgOperand(0))) {
              assert(typeDefCallInst != nullptr && "No type definition found!\n");
              ObjectNode *objectNode = new ObjectNode(callInst, callToTypeNodeMap[typeDefCallInst]);
              this->objects.insert(objectNode);
              this->callToObjectNodeMap[callInst] = objectNode;
            }
          }

          if (callee->getName() == ObjectIRToFunctionNames[BUILD_ARRAY]) {
            if (auto arrayDefCallInst = this->retrieveTypeDefinition(callInst->getArgOperand(0))) {
              assert(arrayDefCallInst != nullptr && "No array type definition found!\n");
              if (auto typeDefCallInst = this->retrieveTypeDefinition(arrayDefCallInst->getArgOperand(0))) {
                assert(typeDefCallInst != nullptr && "No type definition found!\n");
                ObjectNode *objectNode = new ObjectNode(callInst, callToTypeNodeMap[arrayDefCallInst], callToTypeNodeMap[typeDefCallInst]);
                this->objects.insert(objectNode);
                this->callToObjectNodeMap[callInst] = objectNode;
              }
            }
          }
        }
      }
    }
  }

  errs() << "Gathering all field accesses and object deletion\n";
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto callInst = dyn_cast<CallInst>(&I)) {
          auto callee = callInst->getCalledFunction();
          if (!callee) {
            continue;
          }

          if (callee->getName() == "getObjectField" || callee->getName() == "getArrayElement") {
            if (auto *buildObjectCallInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              this->callToObjectNodeMap[buildObjectCallInst]->addFieldAccess(callInst);
            }
          }

          if (callee->getName() == "deleteObject") {
            // deleting struct-like objects
            if (auto *buildObjectCallInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              this->callToObjectNodeMap[buildObjectCallInst]->setObjectDeletion(callInst);
            }
            // deleting array-like objects
            if (auto *castInst = dyn_cast<CastInst>(callInst->getArgOperand(0))) {
              if (auto *buildObjectCallInst = dyn_cast<CallInst>(castInst->getOperand(0))) {
                this->callToObjectNodeMap[buildObjectCallInst]->setObjectDeletion(callInst);
              }
            }
          }
        }
      }
    }
  }

  errs() << "Gathering all reads and writes of all objects fields\n";
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto callInst = dyn_cast<CallInst>(&I)) {
          auto callee = callInst->getCalledFunction();
          if (!callee) {
            continue;
          }

          if (callee->getName().startswith("write")) {
            if (auto *getFieldCallInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              if (auto *buildObjectCallInst = dyn_cast<CallInst>(getFieldCallInst->getArgOperand(0))) {
                this->callToObjectNodeMap[buildObjectCallInst]->addFieldWrite(getFieldCallInst, callInst);
              }
            }
          }
          if (callee->getName().startswith("read")) {
            if (auto *getFieldCallInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              if (auto *buildObjectCallInst = dyn_cast<CallInst>(getFieldCallInst->getArgOperand(0))) {
                this->callToObjectNodeMap[buildObjectCallInst]->addFieldRead(getFieldCallInst, callInst);
              }
            }
          }
        }
      }
    }
  }

  for (auto *objectNode : this->objects) {
    errs() << "===========================\n";
    objectNode->printObjectInfo();
  }
  errs() << "===========================\n";
}

void ObjectAbstraction::transform() {
  // Transform the program
}

CallInst * ObjectAbstraction::retrieveTypeDefinition(Value *value) {
  if (auto *loadTypeInst = dyn_cast<LoadInst>(value)) {
    Value *globalTypePointer = loadTypeInst->getPointerOperand();
    for (auto user : globalTypePointer->users()) {
      if (auto storeTypeInst = dyn_cast<StoreInst>(user)) {
        if (auto typeDefCallInst = dyn_cast<CallInst>(storeTypeInst->getValueOperand())) {
          return typeDefCallInst;
        }
      }
    }
  }

  return nullptr;
}
