#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

/*
 * Field Summary base class implementation
 */
AllocationCode FieldSummary::getCode() const {
  return this->points_to.getCode();
}

AllocationSummary &FieldSummary::pointsTo() const {
  return this->points_to;
}

TypeSummary &FieldSummary::getType() const {
  return this->type;
}

llvm::CallInst &FieldSummary::getCallInst() const {
  return this->call_inst;
}

FieldSummary::FieldSummary(llvm::CallInst &call_inst,
                           AllocationSummary &points_to)
  : call_inst(call_inst),
    points_to(points_to),
    type(points_to.getType()) {
  // Do nothing.
}

/*
 * Struct Field Summary implementation
 */
StructFieldSummary::StructFieldSummary(llvm::CallInst &call_inst,
                                       AllocationSummary &points_to,
                                       uint64_t index)
  : FieldSummary(call_inst, points_to),
    index(index) {
  // Do nothing.
}

uint64_t StructFieldSummary::getIndex() const {
  return this->index;
}

/*
 * Tensor Element Summary implementation
 */
TensorElementSummary::TensorElementSummary(llvm::CallInst &call_inst,
                                           AllocationSummary &points_to,
                                           std::vector<llvm::Value *> &indices)
  : FieldSummary(call_inst, points_to),
    indices(indices) {
  // Do nothing.
}

uint64_t TensorElementSummary::getNumberOfDimensions() const {
  return this->indices.size();
}

llvm::Value &TensorElementSummary::getIndex(uint64_t dimension_index) const {
  assert(dimension_index < this->indices.size()
         && "in TensorElementSummary::getIndex"
         && "dimension out of range of tensor");

  auto value = this->indices.at(dimension_index);
  return *value;
}

} // namespace llvm::memoir
