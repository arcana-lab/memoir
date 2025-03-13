#include <algorithm>

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Assert.hpp"

#include "memoir/ir/TypeCheck.hpp"

namespace llvm::memoir {

// Helper functions.
Type *type_of(llvm::Value &V) {
  return TypeChecker::type_of(V);
}

Type *type_of(MemOIRInst &I) {
  return TypeChecker::type_of(I);
}

// Static getter methods
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

IntegerType &Type::get_u2_type() {
  return IntegerType::get<2, false>();
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
  return IntegerType::get<1, false>();
}

IntegerType &Type::get_size_type(const llvm::DataLayout &DL) {
  auto bitwidth = 8 * DL.getPointerSize(0);
  switch (bitwidth) {
    case 64:
      return IntegerType::get<64, false>();
    case 32:
      return IntegerType::get<32, false>();
  }
  MEMOIR_UNREACHABLE("Unhandled size type bitwidth ", bitwidth);
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

VoidType &Type::get_void_type() {
  return VoidType::get();
}

VoidType &VoidType::get() {
  static VoidType the_type;
  return the_type;
}

ReferenceType &Type::get_ref_type(Type &referenced_type) {
  return ReferenceType::get(referenced_type);
}

ReferenceType &ReferenceType::get(Type &referenced_type) {
  if (ReferenceType::reference_types == nullptr) {
    ReferenceType::reference_types = new map<Type *, ReferenceType *>();
  }

  auto found_type = ReferenceType::reference_types->find(&referenced_type);
  if (found_type != ReferenceType::reference_types->end()) {
    return *(found_type->second);
  }

  auto new_type = new ReferenceType(referenced_type);
  (*ReferenceType::reference_types)[&referenced_type] = new_type;

  return *new_type;
}

map<Type *, ReferenceType *> *ReferenceType::reference_types = nullptr;

#if 0
Type &Type::define(DefineTypeInst &definition, std::string name, Type &type) {
  if (ReferenceType::reference_types == nullptr) {
    ReferenceType::reference_types = new map<Type *, ReferenceType *>();
  }

  auto found_type = ReferenceType::reference_types->find(&referenced_type);
  if (found_type != ReferenceType::reference_types->end()) {
    return *(found_type->second);
  }

  auto new_type = new ReferenceType(referenced_type);
  (*ReferenceType::reference_types)[&referenced_type] = new_type;

  return *new_type;
}

Type &Type::lookup(std::string name) {
  auto found_type = TupleType::defined_types->find(name);
  if (found_type != TupleType::defined_types->end()) {
    return *(found_type->second);
  }

  warnln("No struct definition with name ", name);
  MEMOIR_UNREACHABLE("Could not find a TupleType of the given name");
}

map<std::string, Type *> *Type::defined_types = nullptr;
#endif

// TupleType getter.
TupleType &Type::get_tuple_type(llvm::ArrayRef<Type *> fields) {
  return TupleType::get(fields);
}

TupleType &TupleType::get(llvm::ArrayRef<Type *> fields) {

  if (TupleType::tuple_types == nullptr) {
    TupleType::tuple_types = new ordered_multimap<unsigned, TupleType *>();
  }
  auto &tuple_types = *TupleType::tuple_types;

  auto range = tuple_types.equal_range(fields.size());
  for (auto it = range.first; it != range.second; ++it) {
    auto *type = it->second;
    if (std::equal(type->field_types.begin(),
                   type->field_types.end(),
                   fields.begin(),
                   fields.end())) {
      return *type;
    }
  }

  auto *new_type = new TupleType(fields);
  tuple_types.insert({ fields.size(), new_type });

  return *new_type;
}

ordered_multimap<unsigned, TupleType *> *TupleType::tuple_types = nullptr;

// ArrayType getter.
ArrayType &Type::get_array_type(Type &element_type, size_t length) {
  return ArrayType::get(element_type, length);
}

ArrayType &ArrayType::get(Type &element_type, size_t length) {
  if (ArrayType::array_types == nullptr) {
    ArrayType::array_types = new ordered_multimap<Type *, ArrayType *>();
  }

  auto existing_types = ArrayType::array_types->equal_range(&element_type);
  for (auto it = existing_types.first; it != existing_types.second; ++it) {
    auto *existing_type = it->second;

    if (existing_type->getLength() == length) {
      return *existing_type;
    }
  }

  auto *type = new ArrayType(element_type, length);
  ArrayType::array_types->insert({ &element_type, type });

  return *type;
}

ordered_multimap<Type *, ArrayType *> *ArrayType::array_types = nullptr;

/*
 * AssocArrayType getter
 */
AssocArrayType &AssocArrayType::get(Type &key_type,
                                    Type &value_type,
                                    std::optional<std::string> selection) {
  if (AssocArrayType::assoc_array_types == nullptr) {
    AssocArrayType::assoc_array_types = new AssocArrayType::Types();
  }
  auto &types = *AssocArrayType::assoc_array_types;

  auto &type = types[&key_type][&value_type][selection];

  if (type != nullptr) {
    return *type;
  }

  type = new AssocArrayType(key_type, value_type, selection);

  return *type;
}

AssocArrayType::Types *AssocArrayType::assoc_array_types = nullptr;

/*
 * SequenceType getter
 */
SequenceType &SequenceType::get(Type &element_type,
                                std::optional<std::string> selection) {
  if (SequenceType::sequence_types == nullptr) {
    SequenceType::sequence_types = new SequenceType::Types();
  }
  auto &types = *SequenceType::sequence_types;

  auto &type = types[&element_type][selection];

  if (type != nullptr) {
    return *type;
  }

  type = new SequenceType(element_type, selection);

  return *type;
}

SequenceType::Types *SequenceType::sequence_types = nullptr;

/*
 * Static checker methods
 */
bool Type::is_primitive_type(Type &type) {
  switch (type.getKind()) {
    case TypeKind::INTEGER:
    case TypeKind::FLOAT:
    case TypeKind::DOUBLE:
    case TypeKind::POINTER:
      return true;
    default:
      return false;
  }
}

bool Type::is_reference_type(Type &type) {
  switch (type.getKind()) {
    case TypeKind::REFERENCE:
      return true;
    default:
      return false;
  }
}

bool Type::is_struct_type(Type &type) {
  switch (type.getKind()) {
    case TypeKind::TUPLE:
      return true;
    default:
      return false;
  }
}

bool Type::is_collection_type(Type &type) {
  switch (type.getKind()) {
    case TypeKind::ARRAY:
    case TypeKind::ASSOC_ARRAY:
    case TypeKind::SEQUENCE:
      return true;
    default:
      return false;
  }
}

bool Type::is_unsized(Type &type) {
  switch (type.getKind()) {
    case TypeKind::ASSOC_ARRAY:
    case TypeKind::SEQUENCE:
      return true;
    default:
      return false;
  }
}

bool Type::value_is_object(llvm::Value &value) {
  if (not isa<llvm::PointerType>(value.getType())) {
    return false;
  }

  if (not isa<llvm::Instruction>(&value) and not isa<llvm::Argument>(&value)) {
    return false;
  }

  return isa_and_nonnull<ObjectType>(type_of(value));
}

bool Type::value_is_collection_type(llvm::Value &value) {
  if (not isa<llvm::PointerType>(value.getType())) {
    return false;
  }

  if (not isa<llvm::Instruction>(&value) and not isa<llvm::Argument>(&value)) {
    return false;
  }

  return isa_and_nonnull<CollectionType>(type_of(value));
}

bool Type::value_is_struct_type(llvm::Value &value) {
  if (not isa<llvm::PointerType>(value.getType())) {
    return false;
  }

  if (not isa<llvm::Instruction>(&value) and not isa<llvm::Argument>(&value)) {
    return false;
  }

  return isa_and_nonnull<TupleType>(type_of(value));
}

/*
 * Abstract Type implementation
 */
Type::Type(TypeKind code) : code(code) {
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

opt<std::string> Type::get_code() const {
  return {};
}

TypeKind Type::getKind() const {
  return this->code;
}

llvm::Type *Type::get_llvm_type(llvm::LLVMContext &C) const {
  return nullptr;
}

bool Type::operator==(const Type &other) const {
  return this == &other;
}

bool Type::operator<=(const Type &other) const {
  return this == &other;
}

/*
 * IntegerType implementation
 */
IntegerType::IntegerType(unsigned bitwidth, bool is_signed)
  : Type(TypeKind::INTEGER),
    bitwidth(bitwidth),
    is_signed(is_signed) {
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

  return str;
}

opt<std::string> IntegerType::get_code() const {
  std::string str;
  if (this->getBitWidth() == 1) {
    return "boolean";
  }
  str = (this->isSigned()) ? "i" : "u";
  str += std::to_string(this->getBitWidth());
  return str;
}

llvm::Type *IntegerType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::Type::getIntNTy(C, this->getBitWidth());
}

/*
 * FloatType implementation
 */
FloatType::FloatType() : Type(TypeKind::FLOAT) {
  // Do nothing.
}

std::string FloatType::toString(std::string indent) const {
  std::string str;

  str = "f32";

  return str;
}

opt<std::string> FloatType::get_code() const {
  return "f32";
}

llvm::Type *FloatType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::Type::getFloatTy(C);
}

/*
 * DoubleType implementation
 */
DoubleType::DoubleType() : Type(TypeKind::DOUBLE) {
  // Do nothing.
}

std::string DoubleType::toString(std::string indent) const {
  std::string str;

  str = "f64";

  return str;
}

opt<std::string> DoubleType::get_code() const {
  return "f64";
}

llvm::Type *DoubleType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::Type::getDoubleTy(C);
}

/*
 * PointerType implementation
 */
PointerType::PointerType() : Type(TypeKind::POINTER) {
  // Do nothing.
}

std::string PointerType::toString(std::string indent) const {
  std::string str;

  str = "ptr";

  return str;
}

opt<std::string> PointerType::get_code() const {
  return "ptr";
}

llvm::Type *PointerType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::PointerType::get(C, 0);
}

/*
 * VoidType implementation
 */
VoidType::VoidType() : Type(TypeKind::VOID) {
  // Do nothing.
}

std::string VoidType::toString(std::string indent) const {
  std::string str;

  str = "void";

  return str;
}

opt<std::string> VoidType::get_code() const {
  return "void";
}

llvm::Type *VoidType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::Type::getVoidTy(C);
}

/*
 * ReferenceType implementation
 */
ReferenceType::ReferenceType(Type &referenced_type)
  : Type(TypeKind::REFERENCE),
    referenced_type(referenced_type) {
  // Do nothing.
}

Type &ReferenceType::getReferencedType() const {
  return this->referenced_type;
}

std::string ReferenceType::toString(std::string indent) const {
  std::string str;

  str = "&" + this->getReferencedType().toString();

  return str;
}

opt<std::string> ReferenceType::get_code() const {
  auto ref_code = this->getReferencedType().get_code();
  if (!ref_code) {
    return {};
  }
  return *ref_code + "_ref";
}

llvm::Type *ReferenceType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::PointerType::get(C, 0);
}

/*
 * ObjectType implementation
 */
ObjectType::ObjectType(TypeKind kind) : Type(kind) {}

/*
 * TupleType implementation
 */
TupleType::TupleType(llvm::ArrayRef<Type *> fields)
  : ObjectType(TypeKind::TUPLE),
    field_types(fields) {
  // Do nothing.
}

unsigned TupleType::getNumFields() const {
  return this->field_types.size();
}

Type &TupleType::getFieldType(unsigned field_index) const {
  MEMOIR_ASSERT(
      (field_index < this->getNumFields()),
      "Attempt to get length of out-of-range field index for struct type");

  return *(this->field_types[field_index]);
}

llvm::ArrayRef<Type *> TupleType::fields() const {
  return this->field_types;
}

std::string TupleType::toString(std::string indent) const {
  std::string str = "";

  str += "(";
  bool first = true;
  for (auto *field_type : this->fields()) {
    if (not first) {
      str += " ";
    } else {
      first = false;
    }
    str += field_type->toString();
  }
  str += ")";

  return str;
}

opt<std::string> TupleType::get_code() const {
  std::string str = "T" + std::to_string(this->getNumFields());

  for (auto *field_type : this->fields()) {
    str += "_";
    if (auto field_code = field_type->get_code()) {
      str += field_code.value();
    } else {
      MEMOIR_UNREACHABLE("Unknown field code for type ", *field_type);
    }
  }

  return str;
}

llvm::Type *TupleType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::PointerType::get(C, 0);
}

/*
 * Abstract CollectionType implementation
 */
CollectionType::CollectionType(TypeKind code) : ObjectType(code) {
  // Do nothing.
}

opt<std::string> CollectionType::get_code() const {
  return "collection";
}

opt<std::string> CollectionType::get_selection() const {
  return {};
}

CollectionType &CollectionType::set_selection(opt<std::string> selection) {
  return *this;
}

llvm::Type *CollectionType::get_llvm_type(llvm::LLVMContext &C) const {
  return llvm::PointerType::get(C, 0);
}

/*
 * ArrayType implementation
 */
ArrayType::ArrayType(Type &element_type, size_t length)
  : CollectionType(TypeKind::ARRAY),
    element_type(element_type),
    length(length) {}

Type &ArrayType::getElementType() const {
  return this->element_type;
}

size_t ArrayType::getLength() const {
  return this->length;
}

std::string ArrayType::toString(std::string indent) const {
  std::string str;

  str = "[" + this->element_type.toString(indent) + ";"
        + std::to_string(this->length) + "]";

  return str;
}

/*
 * AssocArrayType implementation
 */
AssocArrayType::AssocArrayType(Type &key_type,
                               Type &value_type,
                               opt<std::string> selection)
  : CollectionType(TypeKind::ASSOC_ARRAY),
    key_type(key_type),
    value_type(value_type),
    selection(selection) {
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
  std::string str = "Assoc";

  if (auto selection = this->get_selection()) {
    str += "{" + selection.value() + "}";
  }

  str += "<" + this->key_type.toString(indent) + ", "
         + this->value_type.toString(indent) + ">";

  return str;
}

opt<std::string> AssocArrayType::get_code() const {
  return this->get_selection().value_or("Assoc") + "_"
         + this->getKeyType().get_code().value() + "_"
         + this->getValueType().get_code().value();
}

opt<std::string> AssocArrayType::get_selection() const {
  return this->selection;
}

CollectionType &AssocArrayType::set_selection(opt<std::string> selection) {
  return AssocArrayType::get(this->getKeyType(),
                             this->getValueType(),
                             selection);
}

bool AssocArrayType::operator<=(const Type &other) const {
  if (this == &other) {
    return true;
  }

  const auto *other_assoc = dyn_cast<AssocArrayType>(&other);
  if (not other_assoc) {
    return false;
  }

  return this->getKeyType() <= other_assoc->getKeyType()
         and this->getValueType() <= other_assoc->getValueType();
}

/*
 * SequenceType implementation
 */
SequenceType::SequenceType(Type &element_type, opt<std::string> selection)
  : CollectionType(TypeKind::SEQUENCE),
    element_type(element_type),
    selection(selection) {
  // Do nothing.
}

Type &SequenceType::getElementType() const {
  return this->element_type;
}

std::string SequenceType::toString(std::string indent) const {
  std::string str = "Seq";

  if (auto selection = this->get_selection()) {
    str += "{" + selection.value() + "}";
  }

  str += "<" + this->element_type.toString() + ">";

  return str;
}

opt<std::string> SequenceType::get_code() const {
  return this->get_selection().value_or("Seq") + "_"
         + this->getElementType().get_code().value();
}

opt<std::string> SequenceType::get_selection() const {
  return this->selection;
}

CollectionType &SequenceType::set_selection(opt<std::string> selection) {
  return SequenceType::get(this->getElementType(), selection);
}

bool SequenceType::operator<=(const Type &other) const {
  if (this == &other) {
    return true;
  }

  const auto *other_seq = dyn_cast<SequenceType>(&other);
  if (not other_seq) {
    return false;
  }

  return this->getElementType() <= other_seq->getElementType();
}

// Decoding.
Type &Type::from_code(std::string code) {
  if (code[0] == 'u') {
    auto bitwidth = std::atoi(&code.c_str()[1]);
    switch (bitwidth) {
      case 64:
        return Type::get_u64_type();
      case 32:
        return Type::get_u32_type();
      case 16:
        return Type::get_u16_type();
      case 8:
        return Type::get_u8_type();
      case 2:
        return Type::get_u2_type();
    }
  } else if (code[0] == 'i') {
    auto bitwidth = std::atoi(&code.c_str()[1]);
    switch (bitwidth) {
      case 64:
        return Type::get_i64_type();
      case 32:
        return Type::get_i32_type();
      case 16:
        return Type::get_i16_type();
      case 8:
        return Type::get_i8_type();
    }
  }

  if (code == "f64") {
    return DoubleType::get();
  } else if (code == "f32") {
    return FloatType::get();
  } else if (code == "ptr") {
    return PointerType::get();
  } else if (code == "boolean") {
    return Type::get_bool_type();
  } else if (code == "void") {
    return Type::get_void_type();
  }

  // Handle reference types.
  std::string suffix = "_ref";
  if (code.length() > suffix.length()
      and std::equal(suffix.rbegin(), suffix.rend(), code.rbegin())) {
    std::string referenced_code(code, 0, code.length() - suffix.length());
    return ReferenceType::get(Type::from_code(referenced_code));
  }

  // TODO: Handle tuple types.

  // TODO: Handle collection types.

  MEMOIR_UNREACHABLE("Unknown type code");
}

} // namespace llvm::memoir
