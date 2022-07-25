#include "common/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

/*
 * Base Type Summary implementation
 */
TypeSummary::TypeSummary(TypeCode code) : code(code) {
  // Do nothing.
}

TypeCode TypeSummary::getCode() {
  return this->code;
}

bool TypeSummary::equals(TypeSummary *other) {
  return this == other;
}

/*
 * Struct Type Summary implementation
 */
StructTypeSummary &StructTypeSummary::get(
    std::string name,
    std::vector<TypeSummary *> &field_types) {
  /*
   * See if this StructType is already defined.
   * If it is, and its fields are not intialized, then initialize it.
   */
  auto found_type_summary = defined_type_summaries.find(name);
  if (found_type_summary != defined_type_summaries.end()) {
    auto defined_type_summary = found_type_summary->second;
    if (defined_type_summary->getNumFields() == 0) {
      defined_type_summary->field_types = field_types;
    }
  }

  auto new_type_summary = new StructTypeSummary(name, field_types);
  defined_type_summaries[name] = new_type_summary;
  return *new_type_summary;
}

StructTypeSummary &StructTypeSummary::get(std::string name) {
  /*
   * See if this StructType is already defined.
   * If it is, return it.
   * Otherwise, create a new stub.
   */
  auto found_type_summary = defined_type_summaries.find(name);
  if (found_type_summary != defined_type_summaries.end()) {
    return *(found_type_summary->second);
  }

  auto empty_fields = std::vector<TypeSummary *>();
  auto new_type_summary = new StructTypeSummary(name, empty_fields);
  defined_type_summaries[name] = new_type_summary;
  return *new_type_summary;
}

StructTypeSummary::StructTypeSummary(std::string name,
                                     std::vector<TypeSummary *> &field_types)
  : name(name),
    field_types(field_types),
    TypeSummary(TypeCode::StructTy) {
  // Do nothing.
}

map<std::string, StructTypeSummary *>
    StructTypeSummary::defined_type_summaries = {};

/*
 * Tensor Type Summary implementation
 */
TensorTypeSummary &TensorTypeSummary::get(TypeSummary &element_type,
                                          uint64_t num_dimensions) {
  auto found_map = tensor_type_summaries.find(&element_type);
  if (found_map != tensor_type_summaries.end()) {
    auto &dimension_map = found_map->second;
    auto found_summary = dimension_map.find(num_dimensions);
    if (found_summary != dimension_map.end()) {
      return *(found_summary->second);
    }
  }

  auto new_summary = new TensorTypeSummary(element_type, num_dimensions);
  tensor_type_summaries[&element_type][num_dimensions] = new_summary;
  return *new_summary;
}

// NOTE: we don't currently handle constant length tensors
// TensorTypeSummary &TensorTypeSummary::get(
//     TypeSummary &element_type,
//     std::vector<uint64_t> &length_of_dimensions) {
//   return new TensorTypeSummary(element_type, length_of_dimensions);
// }

TensorTypeSummary::TensorTypeSummary(TypeSummary &element_type,
                                     uint64_t num_dimensions)
  : element_type(element_type),
    num_dimensions(num_dimensions),
    is_static_length(false),
    TypeSummary(TypeCode::TensorTy) {
  // Do nothing.
}

// NOTE: we don't currently handle constant length tensors
// TensorTypeSummary::TensorTypeSummary(
//     TypeSummary &element_type,
//     std::vector<uint64_t> &length_of_dimensions)
//   : element_type(element_type),
//     num_dimensions(length_of_dimensions.size()),
//     is_static_length(true),
//     length_of_dimensions(length_of_dimensions),
//     TypeSummary(TypeCode::TensorTy) {
//   // Do nothing.
// }

map<TypeSummary *, map<uint64_t, TensorTypeSummary *>>
    TensorTypeSummary::tensor_type_summaries = {};

/*
 * Reference Type Summary implementation
 */
ReferenceTypeSummary &ReferenceTypeSummary::get(TypeSummary &referenced_type) {
  auto found_summary = reference_type_summaries.find(&referenced_type);
  if (found_summary != reference_type_summaries.end()) {
    auto existing_summary = found_summary->second;
    return *existing_summary;
  }

  auto new_summary = new ReferenceTypeSummary(referenced_type);
  reference_type_summaries[&referenced_type] = new_summary;
  return *new_summary;
}

ReferenceTypeSummary::ReferenceTypeSummary(TypeSummary &referenced_type)
  : referenced_type(referenced_type),
    TypeSummary(TypeCode::ReferenceTy) {
  // Do nothing.
}

TypeSummary &ReferenceTypeSummary::getReferencedType() {
  return this->referenced_type;
}

map<TypeSummary *, ReferenceTypeSummary *>
    ReferenceTypeSummary::reference_type_summaries = {};

/*
 * Integer Type Summary implementation
 */
IntegerTypeSummary &IntegerTypeSummary::get(unsigned bitwidth, bool is_signed) {
  auto found_bitwidth = integer_type_summaries.find(bitwidth);
  if (found_bitwidth != integer_type_summaries.end()) {
    auto &is_signed_map = found_bitwidth->second;
    auto found_is_signed = is_signed_map.find(is_signed);
    if (found_is_signed != is_signed_map.end()) {
      auto &existing_summary = found_is_signed->second;
      return *existing_summary;
    }
  }

  auto new_type_summary = new IntegerTypeSummary(bitwidth, is_signed);

  integer_type_summaries[bitwidth][is_signed] = new_type_summary;

  return *new_type_summary;
}

IntegerTypeSummary::IntegerTypeSummary(unsigned bitwidth, bool is_signed)
  : bitwidth(bitwidth),
    is_signed(is_signed),
    TypeSummary(TypeCode::IntegerTy) {
  // Do nothing.
}

unsigned IntegerTypeSummary::getBitWidth() {
  return this->bitwidth;
}

bool IntegerTypeSummary::isSigned() {
  return this->is_signed;
}

map<unsigned, map<bool, IntegerTypeSummary *>>
    IntegerTypeSummary::integer_type_summaries = {};

/*
 * Float Type Summary implementation
 */
FloatTypeSummary &FloatTypeSummary::get() {
  static FloatTypeSummary the_float_type_summary;

  return the_float_type_summary;
}

FloatTypeSummary::FloatTypeSummary() : TypeSummary(TypeCode::FloatTy) {
  // Do nothing.
}

/*
 * Double Type Summary implementation
 */
DoubleTypeSummary &DoubleTypeSummary::get() {
  static DoubleTypeSummary the_double_type_summary;

  return the_double_type_summary;
}

DoubleTypeSummary::DoubleTypeSummary() : TypeSummary(TypeCode::DoubleTy) {
  // Do nothing.
}

} // namespace llvm::memoir
