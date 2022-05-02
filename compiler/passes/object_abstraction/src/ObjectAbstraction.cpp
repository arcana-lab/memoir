#include "ObjectAbstraction.hpp"

using namespace object_abstraction;

TypeNode::TypeNode(CallInst *callInst)
  : definition(callInst) {
  this->numFields = dyn_cast<ConstantInt>(callInst->getArgOperand(0))->getSExtValue();
  for (int i = 1; i < 1 + this->numFields; i++) {
    this->fields.push_back(static_cast<CallInst *>(callInst->getArgOperand(i)));
  }

  return;
}

void TypeNode::printTypeInfo() {
  errs() << "Definition: " << *this->definition << "\n";
  errs() << "  Num of Fields: " << this->numFields << "\n";
  for (int i = 0; i < this->numFields; i++) {
    errs() << "  Field " << i << ": " << *this->fields[i] << "\n";
  }

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
          }
        }
      }
    }
  }

  for (auto *typeNode : this->types) {
      typeNode->printTypeInfo();
  }
}

void ObjectAbstraction::transform() {
  // Transform the program
}
