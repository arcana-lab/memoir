#include "ObjectAbstraction.hpp"

using namespace object_abstraction;

TypeNode::TypeNode(CallInst *callInst)
  : definition(callInst) {
  this->numFields = dyn_cast<ConstantInt>(callInst->getArgOperand(0))->getSExtValue();
  for (int i = 1; i < 1 + this->numFields; i++) {
    this->fieldTypes.push_back(dyn_cast<CallInst>(callInst->getArgOperand(i)));
  }

  return;
}

void TypeNode::printTypeInfo(string prefix="") {
  errs() << prefix << "Definition: " << *this->definition << "\n";
  errs() << prefix << "  Num of Fields: " << this->numFields << "\n";
  for (int i = 0; i < this->numFields; i++) {
    errs() << prefix << "    Field " << i << ": " << *this->fieldTypes[i] << "\n";
  }

  return;
}

FieldNode::FieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType)
  : objectBelongsTo(objectBelongsTo),
    fieldType(fieldType) {
  this->fieldReads = std::unordered_set<CallInst *>();
  this->fieldWrites = std::unordered_set<CallInst *>();
  
  return;
}

void FieldNode::printFieldInfo(string prefix="") {
  errs() << prefix << "Field type: " << *this->fieldType << "\n";
  errs() << prefix << "  Field reads: " << "\n";
  for (auto fieldRead : this->fieldReads) {
    errs() << prefix << "  " << *fieldRead << "\n";
  }
  errs() << prefix << "  Field writes: " << "\n";
  for (auto fieldWrite : this->fieldWrites) {
    errs() << prefix << "  " << *fieldWrite << "\n";
  }

  return;
}

ObjectNode::ObjectNode(CallInst *callInst, TypeNode *typeNode)
  : allocation(callInst),
    manifest(typeNode) {
  this->fields = std::vector<FieldNode *>();
  this->fieldAccessorMap = std::unordered_map<CallInst *, FieldNode *>();
  for (int i = 0; i < typeNode->getNumFields(); i++) {
    FieldNode *fieldNode = new FieldNode(this, typeNode->getFieldType(i));
    this->fields.push_back(fieldNode);
  }

  return;
}

void ObjectNode::addFieldAccessor(CallInst *fieldAccessor) {
  if (auto fieldIndexConstant = dyn_cast<ConstantInt>(fieldAccessor->getArgOperand(1))) {
    this->fieldAccessorMap[fieldAccessor] = this->fields[fieldIndexConstant->getSExtValue()];
  }

  return;
}

void ObjectNode::printObjectInfo(string prefix="") {
  errs() << prefix << "Allocation: " << *this->allocation << "\n";
  errs() << prefix << "Manifest: " << "\n";
  this->manifest->printTypeInfo("  ");
  errs() << prefix << "Field accesses: " << "\n";
  for (auto &pair : this->fieldAccessorMap) {
    errs() << prefix << "  " << *pair.first << "\n";
  }
  errs() << prefix << "Fields: " << "\n";
  for (auto fieldNode : this->fields) {
    fieldNode->printFieldInfo("  ");
  }
  errs() << prefix << "Deallocation: " << *this->deletion << "\n";

  return;
}

ObjectAbstraction::ObjectAbstraction(Module &M, Noelle *noelle, PDG *pdg)
  : M(M),
    noelle(noelle),
    pdg(pdg) {
  // Do initialization.
}

void ObjectAbstraction::analyze() {
  // Analyze the program

  errs() << "Gathering objectIR calls\n";
  for (auto &F : this->M) {
    // Ignore function declaration
    if (F.empty()) {
      continue;
    }
    errs() << "  Inside function: " << F.getName() << "\n";
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto callInst = dyn_cast<CallInst>(&I)) {
          auto callee = callInst->getCalledFunction();

          if (!callee) {
            // This is an indirect call, ignore for now
            continue;
          }

          if (isObjectIRCall(callee->getName())) {
            errs() << "    " << I << "\n";

            this->callsToObjectIR.insert(callInst);
          }
        }
      }
    }
  }

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

          // assumption at the moment
          // the callInst is an object-ir call to buildObject
          if (callee->getName() == ObjectIRToFunctionNames[BUILD_OBJECT]) {
            if (auto *loadTypeInst = dyn_cast<LoadInst>(callInst->getArgOperand(0))) {
              Value *globalTypePointer = loadTypeInst->getPointerOperand();
              for (auto user : globalTypePointer->users()) {
                if (auto storeTypeInst = dyn_cast<StoreInst>(user)) {
                  if (auto callTypeInst = dyn_cast<CallInst>(storeTypeInst->getValueOperand())) {
                    ObjectNode *objectNode = new ObjectNode(callInst, callToTypeNodeMap[callTypeInst]);
                    this->objects.insert(objectNode);
                    this->callToObjectNodeMap[callInst] = objectNode;
                    break;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  errs() << "Gathering all field accessors of all objects\n";
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

          // assumption at the moment
          // the callInst is an object-ir call to getObjectField
          if (callee->getName() == "getObjectField") {
            if (auto *callObjectInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              this->callToObjectNodeMap[callObjectInst]->addFieldAccessor(callInst);
            }
          }
        }
      }
    }
  }

  errs() << "Gathering all object deallocations\n";
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

          // assumption at the moment
          // the callInst is an object-ir call to getObjectField
          if (callee->getName() == "deleteObject") {
            if (auto *callObjectInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              this->callToObjectNodeMap[callObjectInst]->setObjectDeletion(callInst);
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

          // assumption at the moment
          // the field accessors is only for UInt64
          if (callee->getName() == "writeUInt64") {
            if (auto *callFieldInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              if (auto *callObjectInst = dyn_cast<CallInst>(callFieldInst->getArgOperand(0))) {
                this->callToObjectNodeMap[callObjectInst]->addFieldWrite(callFieldInst, callInst);
              }
            }
          }
          if (callee->getName() == "readUInt64") {
            if (auto *callFieldInst = dyn_cast<CallInst>(callInst->getArgOperand(0))) {
              if (auto *callObjectInst = dyn_cast<CallInst>(callFieldInst->getArgOperand(0))) {
                this->callToObjectNodeMap[callObjectInst]->addFieldRead(callFieldInst, callInst);
              }
            }
          }
        }
      }
    }
  }

  for (auto *typeNode : this->types) {
    typeNode->printTypeInfo();
  }

  for (auto *objectNode : this->objects) {
    objectNode->printObjectInfo();
  }
}

void ObjectAbstraction::transform() {
  // Transform the program
}
