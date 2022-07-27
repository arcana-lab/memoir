#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

/*
 * Field Summary base class implementation
 */

FieldSummary::FieldSummary(llvm::CallInst &call_inst, ObjectSummary &points_to)
  : call_inst(call_inst),
    points_to(points_to) {
  // Do nothing.
}

ObjectSummary &FieldSummary::pointsTo() const {
  return this->points_to;
}

TypeCode FieldSummary::getTypeCode() const {
  return this->getType().getCode();
}

AllocationSummary &FieldSummary::getAllocation() const {
  return this->points_to.getAllocation();
}

llvm::CallInst &FieldSummary::getCallInst() const {
  return this->call_inst;
}

/*
 * Struct Field Summary implementation
 */
StructFieldSummary::StructFieldSummary(llvm::CallInst &call_inst,
                                       ObjectSummary &points_to,
                                       uint64_t index)
  : FieldSummary(call_inst, points_to),
    index(index) {
  // Do nothing.
}

uint64_t StructFieldSummary::getIndex() const {
  return this->index;
}

TypeSummary &StructFieldSummary::getType() const {
  auto &points_to_type = this->points_to.getType();
  auto &struct_type = static_cast<StructTypeSummary &>(points_to_type);
  return struct_type.getField(this->index);
}

/*
 * Tensor Element Summary implementation
 */
TensorElementSummary::TensorElementSummary(llvm::CallInst &call_inst,
                                           ObjectSummary &points_to,
                                           std::vector<llvm::Value *> &indices)
  : FieldSummary(call_inst, points_to),
    indices(indices) {
  // Do nothing.
}

uint64_t TensorElementSummary::getNumberOfDimensions() const {
  return this->indices.size();
}

TypeSummary &TensorElementSummary::getType() const {
  auto &points_to_type = this->points_to.getType();
  auto &tensor_type = static_cast<TensorTypeSummary &>(points_to_type);
  return tensor_type.getElementType();
}

llvm::Value &TensorElementSummary::getIndex(uint64_t dimension_index) const {
  assert(dimension_index < this->indices.size()
         && "in TensorElementSummary::getIndex"
            "dimension out of range of tensor");

  auto value = this->indices.at(dimension_index);
  return *value;
}

} // namespace llvm::memoir
