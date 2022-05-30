#include "ObjectAbstraction.hpp"

namespace object_abstraction {

  ObjectNode::ObjectNode(CallInst *callInst, TypeNode *typeNode)
    : allocation(callInst),
      manifest(typeNode) {
    for (int i = 0; i < typeNode->getNumFields(); i++) {
      FieldNode *fieldNode = new FieldNode(this, typeNode->getFieldTypeAtIndex(i));
      this->fields.push_back(fieldNode);
    }

    return;
  }

  ObjectNode::ObjectNode(CallInst *callInst, TypeNode *arrayTypeNode, TypeNode *innerTypeNode) 
    : allocation(callInst),
      manifest(arrayTypeNode) {
    FieldNode *summaryFieldNode = new SummaryFieldNode(this, arrayTypeNode->getDefinition(), innerTypeNode, callInst->getArgOperand(1));
    this->fields.push_back(summaryFieldNode);

    return;
  }

  void ObjectNode::addFieldAccess(CallInst *fieldAccess) {
    if (fieldAccess->getCalledFunction()->getName() == "getObjectField") {   // accessing struct-like object    
      if (auto fieldIndexConstant = dyn_cast<ConstantInt>(fieldAccess->getArgOperand(1))) {
        this->fieldAccessMap[fieldAccess] = this->fields[fieldIndexConstant->getSExtValue()];
      }
    } else {                                                                // accessing array-like object
      this->fieldAccessMap[fieldAccess] = this->fields[0];
    }

    return;
  }

  void ObjectNode::printObjectInfo(string prefix) {
    errs() << prefix << "Object allocation: " << *this->allocation << "\n";

    errs() << prefix << "Manifest: " << "\n";
    this->manifest->printTypeInfo(prefix + "  ");

    errs() << "Object fields:\n";
    for (auto fieldNode : this->fields) {
      fieldNode->printFieldInfo(prefix + "  ");
    }

    errs() << prefix << "Object accesses: " << "\n";
    for (auto &pair : this->fieldAccessMap) {
      errs() << prefix << "  " << *pair.first << "\n";
    }

    errs() << prefix << "Object deallocation:\n";
    if (this->deletions.size() > 0) {
      for (auto &deletion : this->deletions) {
        errs() << prefix << "  " << *deletion << "\n";
      }
    }

    return;
  }

} // namespace object_abstraction