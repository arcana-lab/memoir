#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"

namespace llvm::memoir {

/*
 * ReadInst implementation
 */
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
  auto collection = CollectionAnalysis::analyze(this->getObjectOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the struct being accessed");
  return *collection;
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
  // TODO: flesh out.
  return "struct read";
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
  // TODO: flesh out.
  return "index read";
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
  // TODO: flesh out.
  return "assoc read";
}

/*
 * WriteInst implementation
 */
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
  // TODO: flesh out.
  return "struct write";
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
  // TODO: flesh out.
  return "index write";
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
  // TODO: flesh out.
  return "assoc write";
}

/*
 * GetInst implementation
 */
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
  // TODO: flesh out.
  return "struct get";
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
  // TODO: flesh out.
  return "index get";
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
  // TODO: flesh out.
  return "assoc get";
}

} // namespace llvm::memoir
