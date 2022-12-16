#include "common/ir/Instructions.cpp"

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

llvm::Value &StructReadInst::getFieldIndex() const {
  return this->getFieldIndexAsUse().get();
}

llvm::Use &StructReadInst::getFieldIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
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

llvm::Value &StructWriteInst::getFieldIndex() const {
  return this->getFieldIndexAsUse().get();
}

llvm::Use &StructWriteInst::getFieldIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
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

llvm::Value &StructGetInst::getFieldIndex() const {
  return this->getFieldIndexAsUse().get();
}

llvm::Use &StructGetInst::getFieldIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
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

llvm::Value &AssocGetInst::getKeyOperand() const {
  return this->getKeyOperandAsUse().get();
}

llvm::Use &AssocGetInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandAsUse(1);
}
