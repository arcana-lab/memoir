#include "common/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * CollectionSummary implementation
 */
CollectionSummary::CollectionSummary(CollectionCode code) : code(code) {
  // Do nothing.
}

CollectionCode CollectionSummary::getCode() const {
  return this->code;
}

bool CollectionSummary::operator==(const CollectionSummary &other) const {
  if (this->code != other.code) {
    return false;
  }

  switch (this->getCode()) {
    case CollectionCode::BASE:
      return (static_cast<BaseCollectionSummary &>(*this)
              == static_cast<BaseCollectionSummary &>(other));
    case CollectionCode::FIELD_ARRAY:
      return (static_cast<FieldArraySummary &>(*this)
              == static_cast<FieldArraySummary &>(other));
    case CollectionCode::CONTROL_PHI:
      return (static_cast<ControlPHISummary &>(*this)
              == static_cast<ControlPHISummary &>(other));
    case CollectionCode::DEF_PHI:
      return (static_cast<DefPHISummary &>(*this)
              == static_cast<DefPHISummary &>(other));
    case CollectionCode::USE_PHI:
      return (static_cast<UsePHISummary &>(*this)
              == static_cast<UsePHISummary &>(other));
    default:
      return true;
  }
}

/*
 * BaseCollectionSummary implementation
 */
BaseCollectionSummary::BaseCollectionSummary(AllocationSummary &allocation)
  : allocation(allocation),
    CollectionSummary(CollectionCode::BASE) {
  // Do nothing.
}

AllocationSummary &BaseCollectionSummary::getAllocation() const {
  return this->allocation;
}

bool BaseCollectionSummary::operator==(
    const BaseCollectionSummary &other) const {
  return getAllocation() == other.getAllocation();
}

/*
 * FieldArraySummary implementation
 */
FieldArraySummary::FieldArraySummary(TypeSummary &type, unsigned field_index)
  : type(type),
    field_index(field_index),
    CollectionSummary(CollectionCode::FIELD_ARRAY) {
  // Do nothing.
}

TypeSummary &FieldArraySummary::getType() const {
  return this->type;
}

unsigned FieldArraySummary::getIndex() const {
  return this->field_index;
}

bool FieldArraySummary::operator==(const FieldArraySummary &other) const {
  return (this->getIndex() == other.getIndex())
         && (this->getType() == other.getType());
}

/*
 * ControlPHISummary implementationn
 */
ControlPHISummary::ControlPHISummary(
    llvm::PHINode *phi_node,
    std::vector<std::pair<Collectionsummary *, llvm::BasicBlock *>> &incoming)
  : phi_node(phi_node),
    incoming(incoming),
    CollectionSummary(CollectionCode::CONTROL_PHI) {
  // Do nothing.
}

CollectionSummary &ControlPHISummary::getIncomingCollection(
    unsigned idx) const {
  auto &bb = this->getIncomingBlock(idx);

  return this->getIncomingCollectionForBlock(bb);
}

CollectionSummary &ControlPHISummary::getIncomingCollectionForBlock(
    const llvm::BasicBlock &bb) {
  auto found_incoming = this->incoming.find(&bb);
  if (found_incoming == this->incoming.end()) {
    assert(false
           && "in ControlPHISummary::getIncomingCollectionForBlock"
              "basic block is not an incoming edge for this control PHI");
  }

  return *(found_incoming->second);
}

llvm::BasicBlock &ControlPHISummary::getIncomingBlock(unsigned idx) const {
  assert(idx < this->getNumIncoming()
         && "in ControlPHISummary::getIncomingBlock"
            "index out of range");

  auto bb = this->getPHI().getIncomingBlock(idx);
  assert(bb != nullptr
         && "in ControlPHISummary::getIncomingBlock"
            "no incoming edge at that index");

  return *bb;
}

unsigned ControlPHISummary::getNumIncoming() const {
  return this->getPHI().getNumIncomingValue();
}

bool ControlPHISummary::operator==(const ControlPHISummary &other) const {
  return &(this->getPHI()) == &(other.getPHI());
}

/*
 * DefPHISummary implementation
 */
DefPHISummary::DefPHISummary(CollectionSummary &collection,
                             AccessSummary *access)
  : collection(collection),
    access(access),
    CollectionSummary(CollectionCode::DEF_PHI) {
  // Do nothing.
}

CollectionSummary &DefPHISummary::getCollection() const {
  return this->collection;
}

AccessSummary &DefPHISummary::getAccess() const {
  return this->access;
}

bool DefPHISummary::operator==(const DefPHISummary &other) const {
  return (this->getCollection() == other.getCollection())
         && (this->getAccess() == other.getAccess());
}

/*
 * UsePHISummary implementation
 */
UsePHISummary::UsePHISummary(CollectionSummary &collection,
                             AccessSummary &access)
  : collection(collection),
    access(access),
    CollectionSummary(CollectionCode::USE_PHI) {
  // Do nothing.
}

CollectionSummary &UsePHISummary::getCollection() const {
  return this->collection;
}

AccessSummary &UsePHISummary::getAccess() const {
  return this->access;
}

bool UsePHISummary::operator==(const UsePHISummary &other) const {
  return (this->getCollection() == other.getCollection())
         && (this->getAccess() == other.getAccess());
}

} // namespace llvm::memoir
