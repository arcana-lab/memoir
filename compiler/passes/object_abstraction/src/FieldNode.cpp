#include "ObjectAbstraction.hpp"

namespace object_abstraction {

  FieldNode::FieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType)
    : objectBelongsTo(objectBelongsTo),
      fieldType(fieldType) {

    return;
  }

  void FieldNode::printFieldInfo(string prefix) {
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

  SummaryFieldNode::SummaryFieldNode(ObjectNode *objectBelongsTo, CallInst *fieldType, TypeNode *typeNode, Value *size) 
    : FieldNode(objectBelongsTo, fieldType),
      innerObjectType(typeNode),
      size(size) {

    return;
  }

  void SummaryFieldNode::printFieldInfo(string prefix) {
    errs() << prefix << "Field type: " << *this->fieldType << "\n";
    
    errs() << prefix << "Inner object type:\n";
    this->innerObjectType->printTypeInfo(prefix + "  ");
    
    errs() << prefix << "Field size: " << *this->size << "\n";
    errs() << prefix << "  Writes:\n";
    for (auto fieldWrite : this->fieldWrites) {
      errs() << prefix << "    " << *fieldWrite << "\n";
    }
    errs() << prefix << "  Reads:\n";
    for (auto fieldRead : this->fieldReads) {
      errs() << prefix << "    " << *fieldRead << "\n";
    }

    return ;
  }

} // namespace object_abstraction