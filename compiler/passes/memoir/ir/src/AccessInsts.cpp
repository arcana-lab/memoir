#include "memoir/ir/Instructions.cpp"

/*
 * ReadInst implementation
 */
llvm::Value &ReadInst::getValueRead() const {
  return this->getCallInst();
}

llvm::Value &ReadInst::getObjectOperand() const {
  return this->getObjectOperandAsUse().get();
}

llvm::Use &ReadInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

/*
 * StructReadInst implementation
 */
Collection &StructReadInst::getCollectionAccessed() const {
  return this->getFieldArrayAccessed();
}

FieldArray &StructReadInst::getFieldArray() const {
  return FieldArray::get(this->getStruct()->getType(), this->getFieldIndex());
}

Struct &StructReadInst::getStructAccessed() const {
  // TODO: run the StructAnalysis
  return;
}

unsigned StructReadInst::getFieldIndex() const {
  auto field_index_as_value = this->getFieldIndexOperand();
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
  return this->getFieldIndexAsUse().get();
}

llvm::Use &StructReadInst::getFieldIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}
Collection &IndexReadInst::getCollectionAccessed() const {
  // TODO: run the CollectionAnalysis
  return;
}

unsigned IndexReadInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 1);
}

llvm::Value &IndexReadInst::getIndexOfDimension(unsigned dim_idx) const {
  return this->getIndexOfDimensionAsUse(dim_idx).get();
}

llvm::Use &IndexReadInst::getIndexOfDimensionAsUse(unsigned dim_idx) const {
  return this->getCallInst().getArgOperandUse(1 + dim_idx);
}

Collection &AssocReadInst::getCollectionAccessed() const {
  // TODO: run the CollectionAnalysis
  return;
}

llvm::Value &AssocReadInst::getKeyOperand() const {
  return this->getKeyOperandAsUse().get();
}

llvm::Use &AssocReadInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandAsUse(1);
}

/*
 * WriteInst implementation
 */
llvm::Value &WriteInst::getValueWritten() const {
  return this->getValueWrittenAsUse().get();
}

llvm::Use &WriteInst::getValueWrittenAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &WriteInst::getObjectOperand() const {
  return this->getObjectOperandAsUse().get();
}

llvm::Use &WriteInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

Collection &StructWriteInst::getCollectionAccessed() const {
  return this->getFieldArray();
}

FieldArray &StructWriteInst::getFieldArray() const {
  return FieldArray::get(this->getStruct()->getType(), this->getFieldIndex());
}

Struct &StructWriteInst::getStructAccessed() const {
  // TODO: run the StructAnalysis
  return;
}

unsigned StructWriteInst::getFieldIndex() const {
  auto field_index_as_value = this->getFieldIndexOperand();
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
  return this->getFieldIndexAsUse().get();
}

llvm::Use &StructWriteInst::getFieldIndexOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

Collection &IndexWriteInst::getCollectionAccessed() const {
  // TODO: run the CollectionAnalysis
  return;
}

unsigned IndexWriteInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 2);
}

llvm::Value &IndexWriteInst::getIndexOfDimension(unsigned dim_idx) const {
  return this->getIndexOfDimensionAsUse(dim_idx).get();
}

llvm::Use &IndexWriteInst::getIndexOfDimensionAsUse(unsigned dim_idx) const {
  return this->getCallInst().getArgOperandUse(2 + dim_idx);
}

Collection &AssocWriteInst::getCollectionAccessed() const {
  // TODO: run the CollectionAnalysis
  return;
}

llvm::Value &AssocWriteInst::getKeyOperand() const {
  return this->getKeyOperandAsUse().get();
}

llvm::Use &AssocWriteInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandAsUse(2);
}

/*
 * GetInst implementation
 */
llvm::Value &GetInst::getValueRead() const {
  return this->getCallInst();
}

llvm::Value &GetInst::getObjectOperand() const {
  return this->getObjectOperandAsUse().get();
}

llvm::Use &GetInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Collection &StructGetInst::getCollectionAccessed() const {
  return this->getFieldArray();
}

FieldArray &StructGetInst::getFieldArray() const {
  return FieldArray::get(this->getStruct()->getType(), this->getFieldIndex());
}

Struct &StructGetInst::getStructAccessed() const {
  // TODO: run the StructAnalysis
  return;
}
unsigned StructGetInst::getFieldIndex() const {
  auto field_index_as_value = this->getFieldIndexOperand();
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
  return this->getFieldIndexAsUse().get();
}

llvm::Use &StructGetInst::getFieldIndexOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

Collection &IndexGetInst::getCollectionAccessed() const {
  // TODO: run the CollectionAnalysis
  return;
}

unsigned IndexGetInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 1);
}

llvm::Value &IndexGetInst::getIndexOfDimension(unsigned dim_idx) const {
  return this->getIndexOfDimensionAsUse(dim_idx).get();
}

llvm::Use &IndexGetInst::getIndexOfDimensionAsUse(unsigned dim_idx) const {
  return this->getCallInst().getArgOperandUse(1 + dim_idx);
}

Collection &AssocGetInst::getCollectionAccessed() const {
  // TODO: run the CollectionAnalysis
  return;
}

llvm::Value &AssocGetInst::getKeyOperand() const {
  return this->getKeyOperandAsUse().get();
}

llvm::Use &AssocGetInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandAsUse(1);
}
