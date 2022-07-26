#include "common/analysis/AllocationAnalysis.hpp"

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
 * TensorAllocationSummary implementation
 */
TensorAllocationSummary::TensorAllocationSummary(
    CallInst &call_inst,
    TypeSummary &element_type,
    std::vector<llvm::Value *> &length_of_dimensions)
  : element_type(element_type),
    length_of_dimensions(length_of_dimensions),
    AllocationSummary(
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
  return this->length_of_dimensions[dimension_index];
}

} // namespace llvm::memoir
