#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {
/*
 * ObjectSummary implementation
 */
ObjectSummary::ObjectSummary(llvm::CallInst &call_inst, ObjectCode code)
  : call_inst(call_inst),
    code(code) {
  // Do nothing.
}

bool ObjectSummary::isNested() const {
  switch (this->code) {
    case ObjectCode::NESTED_STRUCT:
      return true;
    default:
      return false;
  }
}

ObjectCode ObjectSummary::getCode() const {
  return this->code;
}

llvm::CallInst &ObjectSummary::getCallInst() const {
  return this->call_inst;
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
    ObjectSummary(allocation.getCallInst(), ObjectCode::BASE) {
  // Do nothing.
}

AllocationSummary &BaseObjectSummary::getAllocation() const {
  return this->allocation;
}

TypeSummary &BaseObjectSummary::getType() const {
  return this->getAllocation().getType();
}

/*
 * NestedStructSummary implementation
 */
NestedStructSummary::NestedStructSummary(llvm::CallInst &call_inst,
                                         FieldSummary &field)
  : field(field),
    ObjectSummary(call_inst, ObjectCode::NESTED_STRUCT) {
  // Do nothing.
}

FieldSummary &NestedStructSummary::getField() const {
  return this->field;
}

AllocationSummary &NestedStructSummary::getAllocation() const {
  return this->getField().getAllocation();
}

TypeSummary &NestedStructSummary::getType() const {
  return this->getField().getType();
}

/*
 * NestedTensorSummary implementation
 */
NestedTensorSummary::NestedTensorSummary(llvm::CallInst &call_inst,
                                         FieldSummary &field)
  : field(field),
    ObjectSummary(call_inst, ObjectCode::NESTED_TENSOR) {
  // Do nothing.
}

FieldSummary &NestedTensorSummary::getField() const {
  return this->field;
}

AllocationSummary &NestedTensorSummary::getAllocation() const {
  return this->getField().getAllocation();
}

TypeSummary &NestedTensorSummary::getType() const {
  return this->getField().getType();
}

} // namespace llvm::memoir
