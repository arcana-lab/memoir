#include "memoir/ir/Types.hpp"
#include "memoir/support/Assert.hpp"

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
 * Static checker methods
 */
bool Type::is_primitive_type(Type &type) {
  switch (type->getCode()) {
    case IntegerTy:
    case FloatTy:
    case DoubleTy:
    case PointerTy:
      return true;
    default:
      return false;
  }
}

bool Type::is_reference_type(Type &type) {
  switch (type->getCode()) {
    case ReferenceTy:
      return true;
    default:
      return false;
  }
}

bool Type::is_struct_type(Type &type) {
  switch (type->getCode()) {
    case StructTy:
      return true;
    default:
      return false;
  }
}

bool Type::is_collection_type(Type &type) {
  switch (type->getCode()) {
    case StaticTensorTy:
    case TensorTy:
    case AssocArrayTy:
    case SequenceTy:
      return true;
    default:
      return false;
  }
}

/*
 * Abstract Type implementation
 */
Type::Type(TypeCode code) : code(code) {
  // Do nothing.
}

std::ostream &operator<<(std::ostream &os, const Type &T) {
  os << T.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Type &T) {
  os << T.toString();
  return os;
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

std::string IntegerType::toString(std::string indent) const {
  std::string str;
  str = "(type: ";
  if (this->getBitWidth() == 1) {
    str += "bool";
  } else {
    if (this->isSigned()) {
      str += "i";
    } else {
      str += "u";
    }
    str += std::to_string(this->getBitWidth());
  }
  str += ")";

  return str;
}

/*
 * FloatType implementation
 */
FloatType::FloatType() : Type(TypeCode::FloatTy) {
  // Do nothing.
}

std::string FloatType::toString(std::string indent) const {
  std::string str;

  str = "(type: f32)";

  return str;
}

/*
 * DoubleType implementation
 */
DoubleType::DoubleType() : Type(TypeCode::DoubleTy) {
  // Do nothing.
}

std::string DoubleType::toString(std::string indent) const {
  std::string str;

  str = "(type: f64)";

  return str;
}

/*
 * PointerType implementation
 */
PointerType::PointerType() : Type(TypeCode::PointerTy) {
  // Do nothing.
}

std::string PointerType::toString(std::string indent) const {
  std::string str;

  str = "(type: ptr)";

  return str;
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

std::string ReferenceType::toString(std::string indent) const {
  std::string str;

  str =
      "(type: ref " + this->getReferencedType().toString("            ") + ")";

  return str;
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

std::string StructType::toString(std::string indent) const {
  std::string str = "";

  str += "(type: struct\n";
  for (auto field_type : this->field_types) {
    auto field_str = field_type->toString(indent + "  ");
    str += indent + "  " + field_str + "\n";
  }
  str += indent + ")";

  return str;
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

std::string StaticTensorType::toString(std::string indent) const {
  std::string str;

  str = "(type: static tensor\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + "  # of dimensions: "
         + std::to_string(this->getNumDimensions()) + "\n";
  for (auto dim = 0; dim < this->length_of_dimensions.size(); dim++) {
    str += indent + "  dimension " + std::to_string(dim) + ": "
           + std::to_string(this->length_of_dimensions.at(dim)) + "\n";
  }
  str += indent + ")";

  return str;
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

std::string TensorType::toString(std::string indent) const {
  std::string str;

  str = "(type: tensor\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + "  # of dimensions: " + std::to_string(this->num_dimensions)
         + "\n";
  str += indent + ")";

  return str;
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

std::string AssocArrayTypeSummary::toString(std::string indent) const {
  std::string str;

  str = "(type: associative array\n";
  str += indent + "  key type: \n";
  str += indent + "    " + this->key_type.toString(indent + "    ") + "\n";
  str += indent + "  value type: \n";
  str += indent + "    " + this->value_type.toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
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

std::string AssocArrayTypeSummary::toString(std::string indent) const {
  std::string str;

  str = "(type: sequence\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}
