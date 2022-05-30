#include "ObjectAbstraction.hpp"

namespace object_abstraction {

  TypeNode::TypeNode(CallInst *callInst)
    : definition(callInst) {
    for (int i = 1; i < callInst->getNumArgOperands(); i++) {
      if (auto fieldTypeCallInst = dyn_cast<CallInst>(callInst->getArgOperand(i))) {
        this->fieldTypes.push_back(fieldTypeCallInst);
      }
    }

    return;
  }

  TypeNode::TypeNode(CallInst *callInst, CallInst *innerTypeCallInst)
    : definition(callInst) {
    this->fieldTypes.push_back(innerTypeCallInst);

    return;
  }

  void TypeNode::printTypeInfo(string prefix) {
    errs() << prefix << "Type definition: " << *this->definition << "\n";
    errs() << prefix << "  Num of fields: " << this->fieldTypes.size() << "\n";
    for (auto fieldType : this->fieldTypes) {
      errs() << prefix << "    Field type: " << *fieldType << "\n";
    }

    return;
  }

} // namespace object_abstraction