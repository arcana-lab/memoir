#include "memoir/analysis/AllocationAnalysis.hpp"

namespace llvm::memoir {

/*
 * AllocationSummary implementation
 */
AllocationSummary::AllocationSummary(CallInst &call_inst,
                                     AllocationCode code,
                                     TypeSummary &type)
  : call_inst(call_inst),
    code(code),
    type(type) {
  // Do nothing.
}

AllocationCode AllocationSummary::getCode() const {
  return this->code;
}

TypeSummary &AllocationSummary::getType() const {
  return this->type;
}

CallInst &AllocationSummary::getCallInst() const {
  return this->call_inst;
}

/*
 * StructAllocationSummary implementation
 */
StructAllocationSummary::StructAllocationSummary(CallInst &call_inst,
                                                 TypeSummary &type)
  : AllocationSummary(call_inst, AllocationCode::STRUCT, type) {
  // Do nothing.
}

/*
 * CollectionAllocationSummary implementationn
 */
CollectionAllocationSummary::CollectionAllocationSummary(
    llvm::CallInst &call_inst,
    AllocationCode code,
    TypeSummary &type)
  : AllocationSummary(call_inst, code, type) {
  // Do nothing.
}

/*
 * TensorAllocationSummary implementation
 */
TensorAllocationSummary::TensorAllocationSummary(
    CallInst &call_inst,
    TypeSummary &element_type,
    std::vector<llvm::Value *> &length_of_dimensions)
  : element_type(element_type),
    length_of_dimensions(length_of_dimensions),
    CollectionAllocationSummary(
        call_inst,
        AllocationCode::TENSOR,
        TensorTypeSummary::get(element_type, length_of_dimensions.size())) {
  // Do nothing.
}

TypeSummary &TensorAllocationSummary::getElementType() const {
  return this->element_type;
}

uint64_t TensorAllocationSummary::getNumberOfDimensions() const {
  return this->length_of_dimensions.size();
}

llvm::Value *TensorAllocationSummary::getLengthOfDimension(
    uint64_t dimension_index) const {
  assert(dimension_index < this->getNumberOfDimensions()
         && "in TensorAllocationSummary::getLengthOfDimension"
            "index out of range");

  auto length_of_dimension = this->length_of_dimensions.at(dimension_index);
  assert((length_of_dimension != nullptr)
         && "in TensorAllocationSummary::getLengthOfDimension"
            "llvm Value for length of dimension is NULL!");

  return *length_of_dimension;
}

/*
 * AssocArrayAllocationSummary implementation
 */
AssocArrayAllocationSummary::AssocArrayAllocationSummary(
    llvm::CallInst &call_inst,
    TypeSummary &key_type,
    TypeSummary &value_type)
  : key_type(key_type),
    value_type(value_type),
    CollectionAllocationSummary(
        call_inst,
        AllocationCode::ASSOC_ARRAY,
        AssocArrayTypeSummary::get(key_type, value_type)) {
  // Do nothing.
}

TypeSummary &AssocArrayAllocationSummary::getKeyType() const {
  return this->key_type;
}

TypeSummary &AssocArrayAllocationSummary::getValueType() const {
  return this->value_type;
}

TypeSummary &AssocArrayAllocationSummary::getElementType() const {
  return this->getValueType();
}

/*
 * SequenceAllocationSummary implementation
 */
SequenceAllocationSummary::SequenceAllocationSummary(llvm::CallInst &call_inst,
                                                     TypeSummary &element_type)
  : element_type(element_type),
    CollectionAllocationSummary(call_inst,
                                AllocationCode::SEQUENCE,
                                SequenceTypeSummary(element_type)) {
  // Do nothing.
}

TypeSummary &SequenceAllocationSummary::getElementType() const {
  return this->element_type;
}

} // namespace llvm::memoir
