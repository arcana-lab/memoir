#include "common/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

/*
 * Struct Type Summary implementation
 */
TypeSummary *StructTypeSummary::get(std::string name,
                                    std::vector<TypeSummary *> &field_types) {
  /*
   * See if this StructType is already defined.
   * If it is, and its fields are not intialized, then initialize it.
   */
  auto found_type_summary = defined_type_summaries.find(name);
  if (found_type_summary != defined_type_summaries.end()) {
    auto defined_type_summary = found_type_summary.second;
    if (defined_type_summary->getNumFields() == 0) {
      defined_type_summary->field_types = field_types;
    }
  }

  return new StructTypeSummary(name, field_types);
}

TypeSummary *StructTypeSummary::get(std::string name) {
  /*
   * See if this StructType is already defined.
   * If it is, return it.
   * Otherwise, create a new stub.
   */
  auto found_type_summary = defined_type_summaries.find(name);
  if (found_type_summary != defined_type_summaries.end()) {
    auto defined_type_summary = found_type_summary.second;
    return defined_type_summary;
  }

  auto new_type_summary = new StructTypeSummary(name, {});
  defined_type_summaries[name] = new_type_summary;
  return new_type_summary;
}

StructTypeSummary::StructTypeSummary(std::string name,
                                     std::vector<TypeSummary *> &field_types)
  : name(name),
    field_types(field_types) {
  // Do nothing.
}

/*
 * Tensor Type Summary implementation
 */
TypeSummary *TensorTypeSummary::get(TypeSummary *element_type,
                                    uint64_t num_dimensions) {
  return new TensorTypeSummary(element_type, num_dimensions);
}

TypeSummary *TensorTypeSummary::get(
    TypeSummary *element_type,
    std::vector<uint64_t> &length_of_dimensions) {
  return new TensorTypeSummary(element_type, length_of_dimensions);
}

TensorTypeSummary::TensorTypeSummary(TypeSummary *element_type,
                                     uint64_t num_dimensions)
  : element_type(element_type),
    num_dimensions(num_dimensions),
    is_static_length(false) {
  // Do nothing.
}

TensorTypeSummary::TensorTypeSummary(
    TypeSummary *element_type,
    std::vector<uint64_t> &length_of_dimensions)
  : element_type(element_type),
    num_dimensions(length_of_dimensions.size()),
    is_static_length(true),
    length_of_dimensions(length_of_dimensions) {
  // Do nothing.
}

/*
 * Reference Type Summary implementation
 */
TypeSummary *ReferenceTypeSummary::get(TypeSummary *referenced_type) {
  return new ReferenceTypeSummary(referenced_type);
}

ReferenceTypeSummary::ReferenceTypeSummary(TypeSummary *referenced_type)
  : referenced_type(referenced_type) {
  // Do nothing.
}

TypeSummary *ReferenceTypeSummary::getReferencedType() {
  return this->reference_type;
}

/*
 * Integer Type Summary implementation
 */
TypeSummary *IntegerTypeSummary::get(unsigned bitwidth, bool is_signed) {
  auto found_bitwidth = integer_type_summaries.find(bitwidth);
  if (found_bitwidth != integer_type_summaries.end()) {
    auto &is_signed_map = found_bitwidth.second;
    auto found_is_signed = is_signed_map.find(is_signed);
    if (found_is_signed != is_signed_map.end()) {
      auto existing_type_summary = found_is_signed.second;
      return existing_type_summary;
    }
  }

  auto new_type_summary = new IntegerTypeSummary(bitwidth, is_signed);

  integer_type_summaries[bitwidth][is_signed] = new_type_summary;

  return new_type_summary;
}

IntegerTypeSummary::IntegerTypeSummary(unsigned bitwidth, bool is_signed)
  : bitwidth(bitwidth),
    is_signed(is_signed) {
  // Do nothing.
}

unsigned IntegerTypeSummary::getBitWidth() {
  return this->bitwidth;
}

bool IntegerTypeSummary::isSigned() {
  return this->is_signed;
}

/*
 * Float Type Summary implementation
 */
TypeSummary *FloatTypeSummary::get() {
  FloatTypeSummary the_float_type_summary;

  return &the_float_type_summary;
}

FloatTypeSummary::FloatTypeSummary() {
  // Do nothing.
}

/*
 * Double Type Summary implementation
 */
TypeSummary *DoubleTypeSummary::get() {
  DoubleTypeSummary the_double_type_summary;

  return &the_double_type_summary;
}

DoubleTypeSummary::DoubleTypeSummary() {
  // Do nothing.
}

} // namespace llvm::memoir
