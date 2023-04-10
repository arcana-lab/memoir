#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

/*
 * ReadInst implementation
 */
CollectionType &ReadInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine type of collection being read");

  auto collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(
      collection_type,
      "Type being accessed by read inst is not a collection type!");

  return *collection_type;
}

llvm::Value &ReadInst::getValueRead() const {
  return this->getCallInst();
}

llvm::Value &ReadInst::getObjectOperand() const {
  return *(this->getObjectOperandAsUse().get());
}

llvm::Use &ReadInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

/*
 * StructReadInst implementation
 */
Collection &StructReadInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getCallInst());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the struct being accessed");
  return *collection;
}

CollectionType &StructReadInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the type being accessed");

  auto struct_type = dyn_cast<StructType>(type);
  MEMOIR_NULL_CHECK(struct_type,
                    "Could not determine the struct type being accessed");

  auto &field_array_type =
      FieldArrayType::get(*struct_type, this->getFieldIndex());

  return field_array_type;
}

Struct &StructReadInst::getStructAccessed() const {
  auto strct = StructAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(strct, "Could not determine the struct being accessed");
  return *strct;
}

unsigned StructReadInst::getFieldIndex() const {
  auto &field_index_as_value = this->getFieldIndexOperand();
  auto field_index_as_constant =
      dyn_cast<llvm::ConstantInt>(&field_index_as_value);
  MEMOIR_NULL_CHECK(field_index_as_constant,
                    "Attempt to access a struct with non-constant field index");

  auto field_index = field_index_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (field_index < 256),
      "Attempt to access a tensor with more than 255 fields"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)field_index;
}

llvm::Value &StructReadInst::getFieldIndexOperand() const {
  return *(this->getFieldIndexOperandAsUse().get());
}

llvm::Use &StructReadInst::getFieldIndexOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string StructReadInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Struct Read: " + llvm_str;

  return str;
}

/*
 * IndexReadInst implementation
 */
Collection &IndexReadInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being accessed");
  return *collection;
}

unsigned IndexReadInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 1);
}

llvm::Value &IndexReadInst::getIndexOfDimension(unsigned dim_idx) const {
  return *(this->getIndexOfDimensionAsUse(dim_idx).get());
}

llvm::Use &IndexReadInst::getIndexOfDimensionAsUse(unsigned dim_idx) const {
  return this->getCallInst().getArgOperandUse(1 + dim_idx);
}

std::string IndexReadInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Index Read: " + llvm_str;

  return str;
}

/*
 * AssocReadInst implementation
 */
Collection &AssocReadInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being accessed");
  return *collection;
}

llvm::Value &AssocReadInst::getKeyOperand() const {
  return *(this->getKeyOperandAsUse().get());
}

llvm::Use &AssocReadInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssocReadInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Assoc Read: " + llvm_str;

  return str;
}

/*
 * WriteInst implementation
 */
CollectionType &WriteInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine type of collection being read");

  auto collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(
      collection_type,
      "Type being accessed by read inst is not a collection type!");

  return *collection_type;
}

llvm::Value &WriteInst::getValueWritten() const {
  return *(this->getValueWrittenAsUse().get());
}

llvm::Use &WriteInst::getValueWrittenAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &WriteInst::getObjectOperand() const {
  return *(this->getObjectOperandAsUse().get());
}

llvm::Use &WriteInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

/*
 * StructWriteInst implementation
 */
Collection &StructWriteInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the struct being accessed");
  return *collection;
}

CollectionType &StructWriteInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the type being accessed");

  auto struct_type = dyn_cast<StructType>(type);
  MEMOIR_NULL_CHECK(struct_type,
                    "Could not determine the struct type being accessed");

  auto &field_array_type =
      FieldArrayType::get(*struct_type, this->getFieldIndex());

  return field_array_type;
}

Struct &StructWriteInst::getStructAccessed() const {
  auto strct = StructAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(strct, "Could not determine struct being accessed");
  return *strct;
}

unsigned StructWriteInst::getFieldIndex() const {
  auto &field_index_as_value = this->getFieldIndexOperand();
  auto field_index_as_constant =
      dyn_cast<llvm::ConstantInt>(&field_index_as_value);
  MEMOIR_NULL_CHECK(field_index_as_constant,
                    "Attempt to access a struct with non-constant field index");

  auto field_index = field_index_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (field_index < 256),
      "Attempt to access a tensor with more than 255 fields"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)field_index;
}

llvm::Value &StructWriteInst::getFieldIndexOperand() const {
  return *(this->getFieldIndexOperandAsUse().get());
}

llvm::Use &StructWriteInst::getFieldIndexOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

std::string StructWriteInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Struct Write: " + llvm_str;

  return str;
}

/*
 * IndexWriteInst implementation
 */
Collection &IndexWriteInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine collection being written to");
  return *collection;
}

unsigned IndexWriteInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 2);
}

llvm::Value &IndexWriteInst::getIndexOfDimension(unsigned dim_idx) const {
  return *(this->getIndexOfDimensionAsUse(dim_idx).get());
}

llvm::Use &IndexWriteInst::getIndexOfDimensionAsUse(unsigned dim_idx) const {
  return this->getCallInst().getArgOperandUse(2 + dim_idx);
}

std::string IndexWriteInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Index Write: " + llvm_str;

  return str;
}

/*
 * AssocWriteInst implementation
 */
Collection &AssocWriteInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine collection being written to");
  return *collection;
}

llvm::Value &AssocWriteInst::getKeyOperand() const {
  return *(this->getKeyOperandAsUse().get());
}

llvm::Use &AssocWriteInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

std::string AssocWriteInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Assoc Write: " + llvm_str;

  return str;
}

/*
 * GetInst implementation
 */
CollectionType &GetInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine type of collection being read");

  auto collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(
      collection_type,
      "Type being accessed by read inst is not a collection type!");

  return *collection_type;
}

llvm::Value &GetInst::getValueRead() const {
  return this->getCallInst();
}

llvm::Value &GetInst::getObjectOperand() const {
  return *(this->getObjectOperandAsUse().get());
}

llvm::Use &GetInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

/*
 * StructGetInst implementation
 */
Collection &StructGetInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the struct being accessed");
  return *collection;
}

CollectionType &StructGetInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the type being accessed");

  auto struct_type = dyn_cast<StructType>(type);
  MEMOIR_NULL_CHECK(struct_type,
                    "Could not determine the struct type being accessed");

  auto &field_array_type =
      FieldArrayType::get(*struct_type, this->getFieldIndex());

  return field_array_type;
}

Struct &StructGetInst::getStructAccessed() const {
  auto strct = StructAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(strct, "Could not determine the struct being accessed");
  return *strct;
}

unsigned StructGetInst::getFieldIndex() const {
  auto &field_index_as_value = this->getFieldIndexOperand();
  auto field_index_as_constant =
      dyn_cast<llvm::ConstantInt>(&field_index_as_value);
  MEMOIR_NULL_CHECK(field_index_as_constant,
                    "Attempt to access a struct with non-constant field index");

  auto field_index = field_index_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (field_index < 256),
      "Attempt to access a tensor with more than 255 fields"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)field_index;
}

llvm::Value &StructGetInst::getFieldIndexOperand() const {
  return *(this->getFieldIndexOperandAsUse().get());
}

llvm::Use &StructGetInst::getFieldIndexOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string StructGetInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Struct Get: " + llvm_str;

  return str;
}

/*
 * IndexGetInst implementation
 */
Collection &IndexGetInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being accessed");
  return *collection;
}

unsigned IndexGetInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 1);
}

llvm::Value &IndexGetInst::getIndexOfDimension(unsigned dim_idx) const {
  return *(this->getIndexOfDimensionAsUse(dim_idx).get());
}

llvm::Use &IndexGetInst::getIndexOfDimensionAsUse(unsigned dim_idx) const {
  return this->getCallInst().getArgOperandUse(1 + dim_idx);
}

std::string IndexGetInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Index Get: " + llvm_str;

  return str;
}

/*
 * AssocGetInst implementation
 */
Collection &AssocGetInst::getCollectionAccessed() const {
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being accessed");
  return *collection;
}

llvm::Value &AssocGetInst::getKeyOperand() const {
  return *(this->getKeyOperandAsUse().get());
}

llvm::Use &AssocGetInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssocGetInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Assoc Get: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
