#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

/*
 * Base Type Summary implementation
 */
TypeSummary::TypeSummary(TypeCode code) : code(code) {
  // Do nothing.
}

TypeCode TypeSummary::getCode() const {
  return this->code;
}

bool TypeSummary::equals(TypeSummary *other) const {
  return this == other;
}

/*
 * Struct Type Summary implementation
 */
StructTypeSummary &StructTypeSummary::get(std::string name,
                                          vector<TypeSummary *> &field_types,
                                          llvm::CallInst &call_inst) {
  /*
   * See if this StructType is already defined.
   * If it is, and its fields are not intialized, then initialize it.
   */
  auto found_type_summary = defined_type_summaries.find(name);
  if (found_type_summary != defined_type_summaries.end()) {
    auto &defined_type_summary = *(found_type_summary->second);
    if (defined_type_summary->getNumFields() == 0) {
      defined_type_summary.field_types = field_types;
      defined_type_summary.call_inst = &call_inst;

      auto field_index = 0;
      for (auto field_type : field_types) {
        auto new_field_array = new FieldArraySummary(field_type,
                                                     defined_type_summary,
                                                     field_index++);
        defined_type_summary.field_arrays.push_back(new_field_array);
      }

      return defined_type_summary;
    }
  }

  auto new_type_summary = new StructTypeSummary(name, field_types, call_inst);

  /*
   * Build the field arrays for this struct type.
   */
  auto field_index = 0;
  for (auto field_type : field_types) {
    auto new_field_array =
        new FieldArraySummary(field_type, defined_type_summary, field_index++);
    new_type_summary->field_arrays.push_back(new_field_array);
  }

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

  auto empty_field_types = vector<TypeSummary *>();
  auto empty_field_arrays = vector<FieldArraySummary *>();
  auto new_type_summary =
      new StructTypeSummary(name, empty_field_types, empty_field_arrays);
  defined_type_summaries[name] = new_type_summary;
  return *new_type_summary;
}

StructTypeSummary::StructTypeSummary(std::string name,
                                     vector<TypeSummary *> &field_types,
                                     vector<FieldArraySummary *> &field_arrays)
  : name(name),
    field_types(field_types),
    field_arrays(field_arrays),
    call_inst(nullptr),
    container(nullptr),
    TypeSummary(TypeCode::StructTy) {
  // Do nothing.
}

StructTypeSummary::StructTypeSummary(std::string name,
                                     vector<TypeSummary *> &field_types,
                                     vector<FieldArraySummary *> &field_arrays,
                                     llvm::CallInst &call_inst)
  : name(name),
    field_types(field_types),
    field_arrays(field_arrays),
    call_inst(&call_inst),
    container(nullptr),
    field_index_of_container(0),
    TypeSummary(TypeCode::StructTy) {
  // Do nothing.
}

StructTypeSummary::StructTypeSummary(std::string name,
                                     vector<TypeSummary *> &field_types,
                                     vector<FieldArraySummary *> &field_arrays,
                                     StructTypeSummary &container,
                                     uint64_t field_index_of_container)
  : name(name),
    field_types(field_types),
    field_arrays(field_arrays),
    call_inst(nullptr),
    container(&container),
    field_index_of_container(field_index_of_container),
    TypeSummary(TypeCode::StructTy) {
  // Do nothing.
}

std::string StructTypeSummary::getName() const {
  return this->name;
}

bool StructTypeSummary::fieldIsANestedStruct(uint64_t field_index) const {
  return !(this->isFieldArray(field_index));
}

TypeSummary &StructTypeSummary::getField(uint64_t field_index) const {
  return *(this->field_types.at(field_index));
}

uint64_t StructTypeSummary::getNumFields() const {
  return this->field_types.size();
}

bool StructTypeSummary::isFieldArray(uint64_t field_index) const {
  return (this->field_arrays.at(field_index) != nullptr);
}

FieldArraySummary &StructTypeSummary::getFieldArray(
    uint64_t field_index) const {
  assert(
      this->isFieldArray(field_index)
      && "in StructTypeSummary::getFieldArray"
         "field index does not contain a field array, did you check StructTypeSummary::isFieldArray?");
  return (this->field_arrays.at(field_index) != nullptr);
}

bool StructTypeSummary::isBase() const {
  return (this->call_inst != nullptr);
}

llvm::CallInst &StructTypeSummary::getCallInst() const {
  assert(this->isBase()
         && "in StructTypeSummary::getCall"
            "call to defineStructType has not been resolved yet");
  return *(this->call_inst);
}

bool StructTypeSummary::isNested() const {
  return (container != nullptr);
}

StructTypeSummary &StructTypeSummary::getContainer() const {
  assert(this->isNested()
         && "in StructTypeSummary::getContainer"
            "trying to get container of non-nested struct type");
  return *(this->container);
}

uint64_t StructTypeSummary::getContainerFieldIndex() const {
  return this->field_index_of_container;
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

TensorTypeSummary::TensorTypeSummary(TypeSummary &element_type,
                                     uint64_t num_dimensions)
  : element_type(element_type),
    num_dimensions(num_dimensions),
    is_static_length(false),
    TypeSummary(TypeCode::TensorTy) {
  // Do nothing.
}

TypeSummary &TensorTypeSummary::getElementType() const {
  return this->element_type;
}

uint64_t TensorTypeSummary::getNumDimensions() const {
  return this->num_dimensions;
}

bool TensorTypeSummary::isStaticLength() const {
  return this->is_static_length;
}

uint64_t TensorTypeSummary::getLengthOfDimension(
    uint64_t dimension_index) const {
  return this->length_of_dimensions.at(dimension_index);
}

map<TypeSummary *, map<uint64_t, TensorTypeSummary *>>
    TensorTypeSummary::tensor_type_summaries = {};

/*
 * Static Tensor Type Summary implementation
 */
StaticTensorTypeSummary::StaticTensorTypeSummary(
    TypeSummary &element_type,
    vector<uint64_t> length_of_dimensions)
  : element_type(element_type),
    length_of_dimensions(num_dimensions),
    TypeSummary(TypeCode::StaticTensorTy) {
  // Do nothing.
}

llvm::CallInst &StaticTensorTypeSummary::getCall() const {
  return this->call_inst;
}

TypeSummary &StaticTensorTypeSummary::getElementType() const {
  return this->element_type;
}

uint64_t StaticTensorTypeSummary::getNumDimensions() const {
  return this->length_of_dimensions.size();
}

uint64_t StaticTensorTypeSummary::getLengthOfDimension(
    uint64_t dimension_index) const {
  return this->length_of_dimensions.at(dimension_index);
}

/*
 * Associative Array Type Summary implementation
 */
AssocArrayTypeSummary &AssocArrayTypeSummary::get(TypeSummary &key_type,
                                                  TypeSummary &value_type) {
  auto found_key_type =
      AssocArrayTypeSummary::assoc_array_type_summaries.find(&key_type);
  if (found_key_type
      != AssocArrayTypeSummary::assoc_array_type_summaries.end()) {
    auto &value_type_map = found_key_type->second;
    auto found_value_type = value_type_map.find(&value_type);
    if (found_value_type != value_type_map.end()) {
      auto &the_assoc_array_type = *(found_value_type->second);
      return the_assoc_array_type;
    }
  }

  auto new_assoc_array_type = new AssocArrayTypeSummary(key_type, value_type);
  AssocArrayTypeSummary::assoc_array_type_summaries[&key_type][&value_type] =
      new_assoc_array_type;
  return *new_assoc_array_type;
}

AssocArrayTypeSummary::AssocArrayTypeSummary(TypeSummary &key_type,
                                             TypeSummary &value_type)
  : key_type(key_type),
    value_type(value_type),
    AssocArrayTypeSummary(TypeCode::AssocArrayTy) {
  // Do nothing.
}

TypeSummary &AssocArrayTypeSummary::getKeyType() const {
  return this->key_type;
}

TypeSummary &AssocArrayTypeSummary::getKeyType() const {
  return this->value_type;
}

map<TypeSummary *, map<TypeSummary *, AssocArrayTypeSummary *>>
    AssocArrayTypeSummary::assoc_array_type_summaries = {};

/*
 * Sequence Type Summary implementation
 */
SequenceTypeSummary &SequenceTypeSummary::get(TypeSummary &element_type) {
  auto found_element_type =
      SequenceTypeSummary::sequence_type_summaries.find(&element_type);
  if (found_element_type
      != SequenceTypeSummary::sequence_type_summaries.end()) {
    auto &the_sequence_type = *(found_element_type->second);
    return the_sequence_type;
  }

  auto new_sequence_type = new SequenceTypeSummary(element_type);
  SequenceTypeSummary::sequence_type_summaries[&element_type] =
      new_sequence_type;
  return *new_sequence_type;
}

SequenceTypeSummary::SequenceTypeSummary(TypeSummary &element_type)
  : element_type(element_type),
    TypeSummary(TypeCode::SequenceTy) {
  // Do nothing.
}

TypeSummary &SequenceTypeSummary() const {
  return this->element_type;
}

map<TypeSummary *, SequenceTypeSummary *>
    SequenceTypeSummary::sequence_type_summaries = {};

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

TypeSummary &ReferenceTypeSummary::getReferencedType() const {
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

unsigned IntegerTypeSummary::getBitWidth() const {
  return this->bitwidth;
}

bool IntegerTypeSummary::isSigned() const {
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

/*
 * Pointer Type Summary implementation
 */
PointerTypeSummary &PointerTypeSummary &get() {
  static PointerTypeSummary the_pointer_type_summary;

  return the_pointer_type_summary;
}

PointerTypeSummary::PointerTypeSummary() : TypeSummary(TypeCode::PointerTy) {
  // Do nothing.
}

} // namespace llvm::memoir
