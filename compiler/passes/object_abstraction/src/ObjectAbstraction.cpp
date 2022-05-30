#include "ObjectAbstraction.hpp"

namespace object_abstraction {

  ObjectAbstraction::ObjectAbstraction(Module &M, Noelle *noelle)
    : M(M),
      noelle(noelle) {
    
    return;
  }

  void ObjectAbstraction::contructObjectAbstractions() {
    this->collectTypeDefinitions();
    this->collectObjectAllocations();
    this->collectObjectAccessesAndDeletions();
    this->collectFieldWritesAndReads();
    this->printTypesAndObjectsInfo();

    return;
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

  void ObjectAbstraction::collectTypeDefinitions() {
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

            if (callee->getName() == ObjectIRToFunctionNames[OBJECT_TYPE]) {
              TypeNode *typeNode = new TypeNode(callInst);
              this->types.insert(typeNode);
              this->callToTypeNodeMap[callInst] = typeNode;
              continue;
            }

            if (callee->getName() == ObjectIRToFunctionNames[ARRAY_TYPE]) {
              if (auto innerTypeCallInst = ObjectAbstraction::retrieveTypeDefinition(callInst->getArgOperand(0))) {
                TypeNode *arrayTypeNode = new TypeNode(callInst, innerTypeCallInst);
                this->types.insert(arrayTypeNode);
                this->callToTypeNodeMap[callInst] = arrayTypeNode;
              }
              continue;
            }
          }
        }
      }
    }

    return;
  }

  void ObjectAbstraction::collectObjectAllocations() {
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
              if (auto typeDefCallInst = ObjectAbstraction::retrieveTypeDefinition(callInst->getArgOperand(0))) {
                assert(typeDefCallInst != nullptr && "No type definition found!\n");
                ObjectNode *objectNode = new ObjectNode(callInst, callToTypeNodeMap[typeDefCallInst]);
                this->objects.insert(objectNode);
                this->callToObjectNodeMap[callInst] = objectNode;
              }
              continue;
            }

            if (callee->getName() == ObjectIRToFunctionNames[BUILD_ARRAY]) {
              if (auto arrayDefCallInst = ObjectAbstraction::retrieveTypeDefinition(callInst->getArgOperand(0))) {
                assert(arrayDefCallInst != nullptr && "No array type definition found!\n");
                if (auto typeDefCallInst = ObjectAbstraction::retrieveTypeDefinition(arrayDefCallInst->getArgOperand(0))) {
                  assert(typeDefCallInst != nullptr && "No type definition found!\n");
                  ObjectNode *arrayObjectNode = new ObjectNode(callInst, callToTypeNodeMap[arrayDefCallInst], callToTypeNodeMap[typeDefCallInst]);
                  this->objects.insert(arrayObjectNode);
                  this->callToObjectNodeMap[callInst] = arrayObjectNode;
                }
              }
              continue;
            }
          }
        }
      }
    }

    return;
  }

  void ObjectAbstraction::collectObjectAccessesAndDeletions() {
    errs() << "Gathering all field accesses and object deletions\n";
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
              continue;
            }

            if (callee->getName() == "deleteObject") {
              // deleting struct-like objects
              if (auto *buildObjectCallInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
                this->callToObjectNodeMap[buildObjectCallInst]->setObjectDeletion(callInst);
              }
              // deleting array-like objects
              if (auto *castInst = dyn_cast<CastInst>(callInst->getArgOperand(0))) {
                if (auto *buildArrayObjectCallInst = dyn_cast<CallInst>(castInst->getOperand(0))) {
                  this->callToObjectNodeMap[buildArrayObjectCallInst]->setObjectDeletion(callInst);
                }
              }
              continue;
            }
          }
        }
      }
    }

    return;
  }

  void ObjectAbstraction::collectFieldWritesAndReads() {
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
              continue;
            }
            if (callee->getName().startswith("read")) {
              if (auto *getFieldCallInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
                if (auto *buildObjectCallInst = dyn_cast<CallInst>(getFieldCallInst->getArgOperand(0))) {
                  this->callToObjectNodeMap[buildObjectCallInst]->addFieldRead(getFieldCallInst, callInst);
                }
              }
              continue;
            }
          }
        }
      }
    }

    return;
  }

  void ObjectAbstraction::printTypesAndObjectsInfo() {
    for (auto *typeNode : this->types) {
      errs() << "=========================================\n";
      typeNode->printTypeInfo();
    }
    errs() << "=========================================\n";

    for (auto *objectNode : this->objects) {
      errs() << "=========================================\n";
      objectNode->printObjectInfo();
    }
    errs() << "=========================================\n";

    return;
  }

} // namespace object_abstraction