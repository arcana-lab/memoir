#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

/*
 * ObjectSummary implementation
 */
ObjectSummary::ObjectSummary(ObjectCode code) : code(code) {
  // Do nothing.
}

ObjectCode ObjectSummary::getCode() const {
  return this->code;
}

bool ObjectSummary::isNested() const {
  switch (this->code) {
    case NESTED:
      return true;
    default:
      return false;
  }
}

AllocationCode ObjectSummary::getAllocationCode() const {
  return this->getAllocation().getCode();
}

TypeCode ObjectSummary::getTypeCode() const {
  return this->getType().getCode();
}

/*
 * BaseObjectSummary implementation
 */
BaseObjectSummary::BaseObjectSummary(AllocationSummary &allocation)
  : allocation(allocation),
    ObjectSummary(ObjectCode::BASE) {
  // Do nothing.
}

AllocationSummary &BaseObjectSummary::getAllocation() const {
  return this->allocation;
}

TypeSummary &BaseObjectSummary::getType() const {
  return this->getAllocation().getType();
}

/*
 * NestedObjectSummary implementation
 */
NestedObjectSummary::NestedObjectSummary(FieldSummary &field)
  : field(field),
    ObjectSummary(ObjectCode::NESTED) {
  // Do nothing.
}

FieldSummary &NestedObjectSummary::getField() const {
  return this->field;
}

AllocationSummary &NestedObjectSummary::getAllocation() const {
  return this->getField().getAllocation();
}

TypeSummary &NestedObjectSummary::getType() const {
  return this->getField().getType();
}

/*
 * ReferencedObjectSummary implementation
 */
ReferencedObjectSummary::ReferencedObjectSummary(llvm::CallInst &call_inst,
                                                 FieldSummary &field)
  : ObjectSummary(ObjectCode::REFERENCED),
    call_inst(call_inst),
    field(field) {
  this->referenced_objects.clear();
}

} // namespace llvm::memoir
