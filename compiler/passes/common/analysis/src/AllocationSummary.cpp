#include "common/analysis/AllocationAnalysis.hpp"

namespace llvm::memoir {

/*
 * AllocationSummary implementation
 */
AllocationSummary::AllocationSummary(AllocationCode code, CallInst &call_inst)
  : code(code),
    call_inst(call_inst) {
  // Do nothing.
}

AllocationCode AllocationSummary::getCode() {
  return this->code;
}

TypeSummary &AllocationSummary::getTypeSummary() {
  return this->type_summary;
}

CallInst &AllocationSummary::getCallInst() {
  return this->call_inst;
}

/*
 * StructAllocationSummary implementation
 */
AllocationSummary &StructAllocationSummary::get(CallInst &call_inst) {
  auto allocation_summary = new StructAllocationSummary(call_inst);

  return *allocation_summary;
}

StructAllocationSummary::StructAllocationSummary(CallInst &call_inst)
  : call_inst(call_inst) {
  // Do nothing.
}

/*
 * TensorAllocationSummary implementation
 */
AllocationSummary &TensorAllocationSummary::get(
    CallInst &call_inst,
    std::vector<llvm::Value *> &length_of_dimensions) {
  auto allocation_summary =
      new TensorAllocationSummary(call_inst, length_of_dimensions);

  return *allocation_summary;
}

TensorAllocationSummary::TensorAllocationSummary(
    CallInst &call_inst,
    std::vector<llvm::Value *> &length_of_dimensions)
  : call_inst(call_inst),
    length_of_dimensions(length_of_dimensions) {
  // Do nothing.
}

} // namespace llvm::memoir
