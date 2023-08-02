#include "memoir/ir/Types.hpp"
#include "memoir/support/Assert.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

/*
 * Static getter methods
 */
IntegerType &Type::get_u64_type() {
  return IntegerType::get<64, false>();
}

IntegerType &Type::get_u32_type() {
  return IntegerType::get<32, false>();
}

IntegerType &Type::get_u16_type() {
  return IntegerType::get<16, false>();
}

IntegerType &Type::get_u8_type() {
  return IntegerType::get<8, false>();
}

IntegerType &Type::get_i64_type() {
  return IntegerType::get<64, true>();
}

IntegerType &Type::get_i32_type() {
  return IntegerType::get<32, true>();
}

IntegerType &Type::get_i16_type() {
  return IntegerType::get<16, true>();
}

IntegerType &Type::get_i8_type() {
  return IntegerType::get<8, true>();
}

IntegerType &Type::get_i2_type() {
  return IntegerType::get<2, true>();
}

IntegerType &Type::get_bool_type() {
  return IntegerType::get<1, true>();
}

template <unsigned BW, bool S>
IntegerType &IntegerType::get() {
  static IntegerType the_type(BW, S);
  return the_type;
}

DoubleType &Type::get_f64_type() {
  return DoubleType::get();
}

DoubleType &DoubleType::get() {
  static DoubleType the_type;
  return the_type;
}

FloatType &Type::get_f32_type() {
  return FloatType::get();
}

FloatType &FloatType::get() {
  static FloatType the_type;
  return the_type;
}

PointerType &Type::get_ptr_type() {
  return PointerType::get();
}

PointerType &PointerType::get() {
  static PointerType the_type;
  return the_type;
}

ReferenceType &Type::get_ref_type(Type &referenced_type) {
  return ReferenceType::get(referenced_type);
}

ReferenceType &ReferenceType::get(Type &referenced_type) {
  auto found_type = ReferenceType::reference_types.find(&referenced_type);
  if (found_type != ReferenceType::reference_types.end()) {
    return *(found_type->second);
  }

  auto new_type = new ReferenceType(referenced_type);
  ReferenceType::reference_types[&referenced_type] = new_type;

  return *new_type;
}

map<Type *, ReferenceType *> ReferenceType::reference_types = {};

StructType &Type::define_struct_type(DefineStructTypeInst &definition,
                                     std::string name,
                                     vector<Type *> field_types) {
  return StructType::define(definition, name, field_types);
}

StructType &Type::get_struct_type(std::string name) {
  return StructType::get(name);
}

StructType &StructType::define(DefineStructTypeInst &definition,
                               std::string name,
                               vector<Type *> field_types) {
  auto found_type = StructType::defined_types.find(name);
  if (found_type != StructType::defined_types.end()) {
    return *(found_type->second);
  }

  auto new_type = new StructType(definition, name, field_types);
  StructType::defined_types[name] = new_type;

  return *new_type;
}

map<std::string, StructType *> StructType::defined_types = {};

StructType &StructType::get(std::string name) {
  auto found_type = StructType::defined_types.find(name);
  if (found_type != StructType::defined_types.end()) {
    return *(found_type->second);
  }

  MEMOIR_UNREACHABLE("Could not find a StructType of the given name");
}

FieldArrayType &Type::get_field_array_type(StructType &struct_type,
                                           unsigned field_index) {
  return FieldArrayType::get(struct_type, field_index);
}

FieldArrayType &FieldArrayType::get(StructType &struct_type,
                                    unsigned field_index) {
  auto found_struct = FieldArrayType::struct_to_field_array.find(&struct_type);
  if (found_struct != FieldArrayType::struct_to_field_array.end()) {
    auto &index_to_field_array = found_struct->second;
    auto found_index = index_to_field_array.find(field_index);
    if (found_index != index_to_field_array.end()) {
      return *(found_index->second);
    }
  }

  auto type = new FieldArrayType(struct_type, field_index);
  FieldArrayType::struct_to_field_array[&struct_type][field_index] = type;

  return *type;
}

map<StructType *, map<unsigned, FieldArrayType *>>
    FieldArrayType::struct_to_field_array = {};

TensorType &Type::get_tensor_type(Type &element_type, unsigned num_dimensions) {
  return TensorType::get(element_type, num_dimensions);
}

TensorType &TensorType::get(Type &element_type, unsigned num_dimensions) {
  auto found_element = TensorType::tensor_types.find(&element_type);
  if (found_element != TensorType::tensor_types.end()) {
    auto &dimensions_to_type_map = found_element->second;
    auto found_dimension = dimensions_to_type_map.find(num_dimensions);
    if (found_dimension != dimensions_to_type_map.end()) {
      return *(found_dimension->second);
    }
  }

  auto type = new TensorType(element_type, num_dimensions);
  TensorType::tensor_types[&element_type][num_dimensions] = type;

  return *type;
}

map<Type *, map<unsigned, TensorType *>> TensorType::tensor_types = {};

/*
 * AssocArrayType getter
 */
AssocArrayType &Type::get_assoc_array_type(Type &key_type, Type &value_type) {
  return AssocArrayType::get(key_type, value_type);
}

AssocArrayType &AssocArrayType::get(Type &key_type, Type &value_type) {
  auto found_key = AssocArrayType::assoc_array_types.find(&key_type);
  if (found_key != AssocArrayType::assoc_array_types.end()) {
    auto &key_to_value_map = found_key->second;
    auto found_value = key_to_value_map.find(&value_type);
    if (found_value != key_to_value_map.end()) {
      return *(found_value->second);
    }
  }

  auto type = new AssocArrayType(key_type, value_type);
  AssocArrayType::assoc_array_types[&key_type][&value_type] = type;

  return *type;
}

map<Type *, map<Type *, AssocArrayType *>>
    AssocArrayType::assoc_array_types = {};

/*
 * SequenceType getter
 */
SequenceType &Type::get_sequence_type(Type &element_type) {
  return SequenceType::get(element_type);
}

SequenceType &SequenceType::get(Type &element_type) {
  auto found_element = SequenceType::sequence_types.find(&element_type);
  if (found_element != SequenceType::sequence_types.end()) {
    return *(found_element->second);
  }

  auto type = new SequenceType(element_type);
  SequenceType::sequence_types[&element_type] = type;
  return *type;
}

map<Type *, SequenceType *> SequenceType::sequence_types = {};

/*
 * Static checker methods
 */
bool Type::is_primitive_type(Type &type) {
  switch (type.getCode()) {
    case TypeCode::INTEGER:
    case TypeCode::FLOAT:
    case TypeCode::DOUBLE:
    case TypeCode::POINTER:
      return true;
    default:
      return false;
  }
}

bool Type::is_reference_type(Type &type) {
  switch (type.getCode()) {
    case TypeCode::REFERENCE:
      return true;
    default:
      return false;
  }
}

bool Type::is_struct_type(Type &type) {
  switch (type.getCode()) {
    case TypeCode::STRUCT:
      return true;
    default:
      return false;
  }
}

bool Type::is_collection_type(Type &type) {
  switch (type.getCode()) {
    case TypeCode::STATIC_TENSOR:
    case TypeCode::TENSOR:
    case TypeCode::ASSOC_ARRAY:
    case TypeCode::SEQUENCE:
      return true;
    default:
      return false;
  }
}

bool Type::value_is_collection_type(llvm::Value &value) {
  auto *type = value.getType();
  auto *ptr_type = dyn_cast<llvm::PointerType>(type);
  if (!ptr_type) {
    return false;
  }
  auto *elem_type = ptr_type->getElementType();
  if (!elem_type) {
    return false;
  }
  auto *elem_struct_type = dyn_cast<llvm::StructType>(elem_type);
  if (!elem_struct_type) {
    return false;
  }
  if (!elem_struct_type->hasName()) {
    return false;
  }
  auto type_name = elem_struct_type->getName();
  return type_name == "struct.memoir::Collection";
}

bool Type::value_is_struct_type(llvm::Value &value) {
  auto *type = value.getType();
  auto *ptr_type = dyn_cast<llvm::PointerType>(type);
  if (!ptr_type) {
    return false;
  }
  auto *elem_type = ptr_type->getElementType();
  if (!elem_type) {
    return false;
  }
  auto *elem_struct_type = dyn_cast<llvm::StructType>(elem_type);
  if (!elem_struct_type) {
    return false;
  }
  if (!elem_struct_type->hasName()) {
    return false;
  }
  auto type_name = elem_struct_type->getName();
  return type_name == "struct.memoir::Struct";
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

TypeCode Type::getCode() const {
  return this->code;
}

/*
 * IntegerType implementation
 */
IntegerType::IntegerType(unsigned bitwidth, bool is_signed)
  : bitwidth(bitwidth),
    is_signed(is_signed),
    Type(TypeCode::INTEGER) {
  // Do nothing.
}

unsigned IntegerType::getBitWidth() const {
  return this->bitwidth;
}

bool IntegerType::isSigned() const {
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
FloatType::FloatType() : Type(TypeCode::FLOAT) {
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
DoubleType::DoubleType() : Type(TypeCode::DOUBLE) {
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
PointerType::PointerType() : Type(TypeCode::POINTER) {
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
ReferenceType::ReferenceType(Type &referenced_type)
  : referenced_type(referenced_type),
    Type(TypeCode::REFERENCE) {
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
StructType::StructType(DefineStructTypeInst &definition,
                       std::string name,
                       vector<Type *> field_types)
  : definition(definition),
    name(name),
    field_types(field_types),
    Type(TypeCode::STRUCT) {
  // Do nothing.
}

DefineStructTypeInst &StructType::getDefinition() const {
  return this->definition;
}

std::string StructType::getName() const {
  return this->name;
}

unsigned StructType::getNumFields() const {
  return this->field_types.size();
}

Type &StructType::getFieldType(unsigned field_index) const {
  MEMOIR_ASSERT(
      (field_index < this->getNumFields()),
      "Attempt to get length of out-of-range field index for struct type");

  return *(this->field_types[field_index]);
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
 * FieldArrayType implementation
 */
FieldArrayType::FieldArrayType(StructType &struct_type, unsigned field_index)
  : struct_type(struct_type),
    field_index(field_index),
    CollectionType(TypeCode::FIELD_ARRAY) {
  // Do nothing.
}

Type &FieldArrayType::getElementType() const {
  return this->getStructType().getFieldType(this->getFieldIndex());
}

StructType &FieldArrayType::getStructType() const {
  return this->struct_type;
}

unsigned FieldArrayType::getFieldIndex() const {
  return this->field_index;
}

std::string FieldArrayType::toString(std::string indent) const {
  std::string str = "";

  str += "(type: field array\n";
  str += indent + "  struct type: \n";
  str += indent + "    " + this->getStructType().toString(indent + "    ");
  str += "\n";
  str += indent + "  field index: \n";
  str += indent + "    " + std::to_string(this->getFieldIndex());
  str += "\n";
  str += indent + ")\n";

  return str;
}

/*
 * StaticTensorType implementation
 */
StaticTensorType::StaticTensorType(Type &element_type,
                                   unsigned number_of_dimensions,
                                   vector<size_t> length_of_dimensions)
  : element_type(element_type),
    number_of_dimensions(number_of_dimensions),
    length_of_dimensions(length_of_dimensions),
    CollectionType(TypeCode::STATIC_TENSOR) {
  // Do nothing.
}

Type &StaticTensorType::getElementType() const {
  return this->element_type;
}

unsigned StaticTensorType::getNumberOfDimensions() const {
  return this->number_of_dimensions;
}

size_t StaticTensorType::getLengthOfDimension(unsigned dimension_index) const {
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
         + std::to_string(this->getNumberOfDimensions()) + "\n";
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
TensorType::TensorType(Type &element_type, unsigned number_of_dimensions)
  : element_type(element_type),
    number_of_dimensions(number_of_dimensions),
    CollectionType(TypeCode::TENSOR) {
  // Do nothing.
}

Type &TensorType::getElementType() const {
  return this->element_type;
}

unsigned TensorType::getNumberOfDimensions() const {
  return this->getNumberOfDimensions();
}

std::string TensorType::toString(std::string indent) const {
  std::string str;

  str = "(type: tensor\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + "  # of dimensions: "
         + std::to_string(this->getNumberOfDimensions()) + "\n";
  str += indent + ")";

  return str;
}

/*
 * AssocArrayType implementation
 */
AssocArrayType::AssocArrayType(Type &key_type, Type &value_type)
  : key_type(key_type),
    value_type(value_type),
    CollectionType(TypeCode::ASSOC_ARRAY) {
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

std::string AssocArrayType::toString(std::string indent) const {
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
    CollectionType(TypeCode::SEQUENCE) {
  // Do nothing.
}

Type &SequenceType::getElementType() const {
  return this->element_type;
}

std::string SequenceType::toString(std::string indent) const {
  std::string str;

  str = "(type: sequence\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}

} // namespace llvm::memoir
