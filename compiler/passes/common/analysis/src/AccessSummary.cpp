#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

/*
 * AccessSummary base class implementation
 */
AccessSummary::AccessSummary(AccessCode code,
                             CallInst &call_inst,
                             CollectionSummary &collection_accessed)
  : code(code),
    call_inst(call_inst),
    collection_accessed(collection_accessed) {
  // Do nothing.
}

CollectionSummary &AccessSummary::getCollection() const {
  return this->collection_accessed;
}

llvm::CallInst &AccessSummary::getCallInst() const {
  return this->call_inst;
}

TypeSummary &AccessSummary::getType() const {
  return this->getCollection().getElementType();
}

bool AccessSummary::isRead() const {
  return !(this->isWrite());
}

bool AccessSummary::isWrite() const {
  return (this->access_info & AccessMask::WRITE_MASK);
}

bool AccessSummary::isAssociative() const {
  return (this->access_info & AccessMask::ASSOC_MASK);
}

bool AccessSummary::isIndexed() const {
  return (this->access_info & AccessMask::INDEXED_MASK);
}

bool AccessSummary::isSlice() const {
  return (this->access_info & AccessMask::SLICE_MASK);
}

AccessInfo AccessSummary::getAccessInfo() const {
  return this->access_info;
}

/*
 * ReadSummary implementation
 */
ReadSummary::ReadSummary(AccessMask mask,
                         llvm::CallInst &call_inst,
                         CollectionSummary &collection_accessed,
                         llvm::Value &value_read)
  : value_read(value_read),
    AccessSummary((AccessMask::READ_MASK | mask),
                  call_inst,
                  collection_accessed) {
  // Do nothing.
}

llvm::Value &ReadSummary::getValueRead() const {
  return this->value_read;
}

/*
 *  WriteSummary implementation
 */
WriteSummary::WriteSummary(AccessMask mask,
                           llvm::CallInst &call_inst,
                           CollectionSummary &collection_accessed,
                           llvm::Value &value_written)
  : value_written(value_written),
    AccessSummary((AccessMask::WRITE_MASK | mask),
                  call_inst,
                  collection_accessed) {
  // Do nothing.
}

llvm::Value &WriteSummary::getValueWritten() const {
  return this->value_written;
}

/*
 * IndexedReadSummary implementation
 */
IndexedReadSummary::IndexedReadSummary(llvm::CallInst &call_inst,
                                       CollectionSummary &collection_accessed,
                                       llvm::Value &value_read,
                                       vector<llvm::Value *> &indices)
  : indices(indices),
    WriteSummary(AccessMask::INDEXED_MASK,
                 call_inst,
                 collection_accessed,
                 value_read) {
  // Do nothing.
}

uint64_t IndexedReadSummary::getNumDimensions() const {
  return this->indices.size();
}

llvm::Value &IndexedReadSummary::getIndexValue(uint64_t dim_idx) const {
  assert((dim_idx < this->getNumDimensions())
         && "in IndexedReadSummary::getIndexValue"
            "index out of range!");

  auto index = this->indices.at(dim_idx);
  assert((index != nullptr)
         && "in IndexedReadSummary::getIndexValue"
            "llvm Value at index is NULL!");

  return *index;
}

/*
 * IndexedWriteSummary implementation
 */
IndexedWriteSummary::IndexedWriteSummary(llvm::CallInst &call_inst,
                                         CollectionSummary &collection_accessed,
                                         llvm::Value &value_read,
                                         vector<llvm::Value *> &indices)
  : indices(indices),
    WriteSummary(AccessMask::INDEXED_MASK,
                 call_inst,
                 collection_accessed,
                 value_read) {
  // Do nothing.
}

uint64_t IndexedWriteSummary::getNumDimensions() const {
  return this->indices.size();
}

llvm::Value &IndexedWriteSummary::getIndexValue(uint64_t dim_idx) const {
  assert((dim_idx < this->getNumDimensions())
         && "in IndexedWriteSummary::getIndexValue"
            "index out of range!");

  auto index = this->indices.at(dim_idx);
  assert((index != nullptr)
         && "in IndexedWriteSummary::getIndexValue"
            "llvm Value at index is NULL!");

  return *index;
}

/*
 * AssocReadSummary implementation
 */
AssocReadSummary::AssocReadSummary(llvm::CallInst &call_inst,
                                   CollectionSummary &collection_accessed,
                                   llvm::Value &value_read,
                                   StructSummary &key)
  : key(key),
    ReadSummary(AccessMask::ASSOC_MASK,
                call_inst,
                collection_accessed,
                value_read) {
  // Do nothing.
}

StructSummary &getKey() const {
  return this->key;
}

TypeSummary &getKeyType() const {
  return this->getKey()->getType();
}

/*
 * AssocWriteSummary implementation
 */
AssocWriteSummary::AssocWriteSummary(llvm::CallInst &call_inst,
                                     CollectionSummary &collection_accessed,
                                     llvm::Value &value_written,
                                     StructSummary &key)
  : key(key),
    WriteSummary(AccessMask::ASSOC_MASK,
                 call_inst,
                 collection_accessed,
                 value_written) {
  // Do nothing.
}

StructSummary &getKey() const {
  return this->key();
}

TypeSummary &getKeyType() const {
  return this->getKey().getType();
}

/*
 * SliceReadSummary implementation
 */
SliceReadSummary::SliceReadSummary(llvm::CallInst &call_inst,
                                   CollectionSummary &collection_accessed,
                                   llvm::Value &value_read,
                                   llvm::Value &left,
                                   llvm::Value &right)
  : left(left),
    right(right),
    ReadSummary(AccessMask::SLICE_MASK,
                call_inst,
                collection_accessed,
                value_read) {
  // Do nothing.
}

llvm::Value &SliceReadSummary::getLeft() const {
  return this->left;
}

llvm::Value &SliceReadSummary::getRight() const {
  return this->right;
}

/*
 * SliceWriteSummary implementation
 */
SliceWriteSummary::SliceWriteSummary(llvm::CallInst &call_inst,
                                     CollectionSummary &collection_accessed,
                                     llvm::Value &value_written,
                                     llvm::Value &left,
                                     llvm::Value &right)
  : left(left),
    right(right),
    WriteSummary(AccessMask::SLICE_MASK,
                 call_inst,
                 collection_accessed,
                 value_written) {
  // Do nothing.
}

llvm::Value &SliceWriteSummary::getLeft() const {
  return this->left;
}

llvm::Value &SliceWriteSummary::getRight() const {
  return this->right;
}

} // namespace llvm::memoir
