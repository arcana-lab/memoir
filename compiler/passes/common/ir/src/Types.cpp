#include "common/ir/Types.hpp"
#include "common/support/Assert.hpp"

/*
 * Static getter methods
 */
IntegerType &Type::get_u64_type() {
  static IntegerType the_type(64, false);
  return the_type;
}

IntegerType &Type::get_u32_type() {
  static IntegerType the_type(32, false);
  return the_type;
}

IntegerType &Type::get_u16_type() {
  static IntegerType the_type(16, false);
  return the_type;
}

IntegerType &Type::get_u8_type() {
  static IntegerType the_type(8, false);
  return the_type;
}

IntegerType &Type::get_i64_type() {
  static IntegerType the_type(64, true);
  return the_type;
}

IntegerType &Type::get_i32_type() {
  static IntegerType the_type(32, true);
  return the_type;
}

IntegerType &Type::get_i16_type() {
  static IntegerType the_type(16, true);
  return the_type;
}

IntegerType &Type::get_i8_type() {
  static IntegerType the_type(8, true);
  return the_type;
}

IntegerType &Type::get_i1_type() {
  static IntegerType the_type(1, true);
  return the_type;
}

DoubleType &Type::get_f64_type() {
  static DoubleType the_type();
  return the_type;
}

FloatType &Type::get_f32_type() {
  static FloatType the_type();
  return the_type;
}

ReferenceType &Type::get_ref_type(Type &referenced_type) {
  return ReferenceType::get(referenced_type);
}

ReferenceType &ReferenceType::get(Type &referenced_type) {
  auto found_type = ReferenceType::reference_types.find(&reference_types);
  if (found_type != ReferenceType::reference_types.end()) {
    return *(found_type->second);
  }

  auto new_type = new ReferenceType(referenced_type);
  ReferenceType::reference_types[&referenced_type] = new_type;

  return *new_type;
}

StructType &Type::get_struct_type(std::string *name) {
  return StructType::get(name);
}

StructType &StructType::define(std::string name,
                               llvm::CallInst &call_inst,
                               vector<Type *> field_types) {
  auto found_type = StructType::defined_types.find(name);
  if (found_type != StructType::defined_types.end()) {
    return *(found_type->second);
  }

  auto new_type = new StructType(name, call_inst, field_types);
  StructType::defined_types[name] = new_type;

  return *new_type;
}

StructType &StructType::get(std::string name) {
  auto found_type = StructType::defined_types.find(name);
  if (found_type != StructType::defined_types.end()) {
    return *(found_type->second);
  }

  MEMOIR_UNREACHABLE("Could not find a StructType of the given name");

  return;
}

/*
 * Abstract Type implementation
 */
Type::Type(TypeCode code) : code(code) {
  // Do nothing.
}

/*
 * IntegerType implementation
 */
IntegerType::IntegerType(unsigned bitwidth, bool is_signed)
  : bitwidth(bitwidth),
    is_signed(is_signed),
    Type(TypeCode::IntegerTy) {
  // Do nothing.
}

unsigned IntegerType::getBitWidth() const {
  return this->bitwidth;
}

bool IntegerType::is_signed() const {
  return this->is_signed;
}

/*
 * FloatType implementation
 */
FloatType::FloatType() : Type(TypeCode::FloatTy) {
  // Do nothing.
}

/*
 * DoubleType implementation
 */
DoubleType::DoubleType() : Type(TypeCode::DoubleTy) {
  // Do nothing.
}

/*
 * PointerType implementation
 */
PointerType::PointerType() : Type(TypeCode::PointerTy) {
  // Do nothing.
}

/*
 * ReferenceType implementation
 */
ReferenceType::ReferenceType(Type &type)
  : type(type),
    Type(TypeCode::ReferenceTy) {
  // Do nothing.
}

Type &ReferenceType::getReferencedType() const {
  return this->referenced_type;
}

/*
 * StructType implementation
 */
StructType::StructType(llvm::CallInst &call_inst,
                       std::string name,
                       vector<Type *> field_types)
  : call_inst(call_inst),
    name(name),
    field_types(field_types),
    Type(TypeCode::StructTy) {
  // Do nothing.
}

llvm::CallInst &StructType::getCallInst() const {
  return this->call_inst;
}

std::string StructType::getName() const {
  return this->name;
}

size_t StructType::getNumFields() const {
  return this->field_types.size();
}

Type &StructType::getFieldType(size_t field_index) const {
  MEMOIR_ASSERT(
      (field_index < this->getNumFields()),
      "Attempt to get length of out-of-range field index for struct type");
}

/*
 * Abstract CollectionType implementation
 */
CollectionType::CollectionType(TypeCode code) : Type(code) {
  // Do nothing.
}

/*
 * StaticTensorType implementation
 */
StaticTensorType::StaticTensorType(Type &element_type,
                                   size_t number_of_dimensions,
                                   vector<size_t> length_of_dimensions)
  : element_type(element_type),
    number_of_dimensions(number_of_dimensions),
    length_of_dimensions(length_of_dimensions),
    CollectionType(TypeCode::StaticTensorTy) {
  // Do nothing.
}

Type &StaticTensorType::getElementType() const {
  return this->element_type;
}

size_t StaticTensorType::getNumberOfDimensions() const {
  return this->number_of_dimensions;
}

size_t StaticTensorType::getLengthOfDimension(size_t dimension_index) const {
  MEMOIR_ASSERT(
      (dimension_index < this->getNumberOfDimensions()),
      "Attempt to get length of out-of-range dimension index for static tensor type");

  return this->length_of_dimensions.at(dimension_index);
}

/*
 * TensorType implementation
 */
TensorType::TensorType(Type &element_type, size_t number_of_dimensions)
  : element_type(element_type),
    number_of_dimensions(number_of_dimensions),
    CollectionType(TypeCode::TensorTy) {
  // Do nothing.
}

Type &TensorType::getElementType() const {
  return this->element_type;
}

size_t TensorType::getNumberOfDimensions() const {
  return this->getNumberOfDimensions();
}

/*
 * AssocArrayType implementation
 */
AssocArrayType::AssocArrayType(Type &key_type, Type &value_type)
  : key_type(key_type),
    value_type(value_type),
    CollectionType(TypeCode::AssocArrayType) {
  // Do nothing.
}

Type &AssocArrayType::getKeyType() const {
  return this->key_type;
}

Type &AssocArrayType::getValueType() const {
  return this->value_type;
}

Type &AssocArrayType::getElementType() const {
  return this->getValueType();
}

/*
 * SequenceType implementation
 */
SequenceType::SequenceType(Type &element_type)
  : element_type(element_type),
    CollectionType(TypeCode::SequenceTy) {
  // Do nothing.
}

Type &SequenceType::getElementType() const {
  return this->element_type;
}