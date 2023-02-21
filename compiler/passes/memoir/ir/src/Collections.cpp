#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * Collection implementation
 */
Collection::Collection(CollectionKind code) : code(code) {
  // Do nothing.
}

CollectionKind Collection::getKind() const {
  return this->code;
}

bool Collection::operator==(const Collection &other) const {
  if (this->getKind() != other.getKind()) {
    return false;
  }

  switch (this->getKind()) {
    case CollectionKind::BASE:
      return (static_cast<const BaseCollection &>(*this)
              == static_cast<const BaseCollection &>(other));
    case CollectionKind::FIELD_ARRAY:
      return (static_cast<const FieldArray &>(*this)
              == static_cast<const FieldArray &>(other));
    case CollectionKind::NESTED:
      return (static_cast<const NestedCollection &>(*this)
              == static_cast<const NestedCollection &>(other));
    case CollectionKind::REFERENCED:
      return (static_cast<const ReferencedCollection &>(*this)
              == static_cast<const ReferencedCollection &>(other));
    case CollectionKind::CONTROL_PHI:
      return (static_cast<const ControlPHICollection &>(*this)
              == static_cast<const ControlPHICollection &>(other));
    case CollectionKind::RET_PHI:
      return (static_cast<const RetPHICollection &>(*this)
              == static_cast<const RetPHICollection &>(other));
    case CollectionKind::ARG_PHI:
      return (static_cast<const ArgPHICollection &>(*this)
              == static_cast<const ArgPHICollection &>(other));
    case CollectionKind::DEF_PHI:
      return (static_cast<const DefPHICollection &>(*this)
              == static_cast<const DefPHICollection &>(other));
    case CollectionKind::USE_PHI:
      return (static_cast<const UsePHICollection &>(*this)
              == static_cast<const UsePHICollection &>(other));
    case CollectionKind::JOIN_PHI:
      return (static_cast<const JoinPHICollection &>(*this)
              == static_cast<const JoinPHICollection &>(other));
    case CollectionKind::SLICE:
      return (static_cast<const SliceCollection &>(*this)
              == static_cast<const SliceCollection &>(other));
    default:
      return true;
  }
}

std::ostream &operator<<(std::ostream &os, const Collection &C) {
  os << C.toString("");
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Collection &C) {
  os << C.toString("");
  return os;
}

/*
 * BaseCollection implementation
 */
BaseCollection::BaseCollection(CollectionAllocInst &allocation)
  : allocation(allocation),
    Collection(CollectionKind::BASE) {
  // Do nothing.
}

CollectionAllocInst &BaseCollection::getAllocation() const {
  return this->allocation;
}

CollectionType &BaseCollection::getType() const {
  return this->getAllocation().getCollectionType();
}

Type &BaseCollection::getElementType() const {
  return this->getType().getElementType();
}

bool BaseCollection::operator==(const BaseCollection &other) const {
  return &(this->getAllocation()) == &(other.getAllocation());
}

std::string BaseCollection::toString(std::string indent) const {
  return "base collection";
}

/*
 * FieldArray implementation
 */
FieldArray &FieldArray::get(StructType &struct_type, unsigned field_index) {
  return FieldArray::get(FieldArrayType::get(struct_type, field_index));
}

map<FieldArrayType *, FieldArray *>
    FieldArray::field_array_type_to_field_array = {};

FieldArray &FieldArray::get(FieldArrayType &type) {
  auto found = FieldArray::field_array_type_to_field_array.find(&type);
  if (found != FieldArray::field_array_type_to_field_array.end()) {
    return *(found->second);
  }

  auto new_field_array = new FieldArray(type);
  FieldArray::field_array_type_to_field_array[&type] = new_field_array;

  return *new_field_array;
}

FieldArray::FieldArray(FieldArrayType &field_array_type)
  : type(field_array_type),
    Collection(CollectionKind::FIELD_ARRAY) {
  // Do nothing.
}

CollectionType &FieldArray::getType() const {
  return this->type;
}

StructType &FieldArray::getStructType() const {
  return this->type.getStructType();
}

unsigned FieldArray::getFieldIndex() const {
  return this->type.getFieldIndex();
}

Type &FieldArray::getElementType() const {
  return this->type.getElementType();
}

bool FieldArray::operator==(const FieldArray &other) const {
  return (this->getFieldIndex() == other.getFieldIndex())
         && (&(this->getType()) == &(other.getType()));
}

std::string FieldArray::toString(std::string indent) const {
  return "field array";
}

/*
 * NestedCollection implementation
 */
NestedCollection::NestedCollection(GetInst &access)
  : access(access),
    Collection(CollectionKind::NESTED) {
  // Do nothing.
}

GetInst &NestedCollection::getAccess() const {
  return this->access;
}

Collection &NestedCollection::getNestingCollection() const {
  return this->getAccess().getCollectionAccessed();
}

CollectionType &NestedCollection::getType() const {
  auto &nesting_type = this->getNestingCollection().getType();

  auto &nested_type = nesting_type.getElementType();
  MEMOIR_ASSERT((Type::is_collection_type(nested_type)),
                "Attempt to construct NestedCollection for access to collection"
                " of non-collection element type");

  return static_cast<CollectionType &>(nested_type);
}

Type &NestedCollection::getElementType() const {
  return this->getType().getElementType();
}

bool NestedCollection::operator==(const NestedCollection &other) const {
  return (&(this->getAccess()) != &(other.getAccess()));
}

std::string NestedCollection::toString(std::string indent) const {
  return "nested collection";
}

/*
 * ReferencedCollection implementation
 */
ReferencedCollection::ReferencedCollection(ReadInst &access)
  : access(access),
    Collection(CollectionKind::REFERENCED) {
  // Do nothing.
}

ReadInst &ReferencedCollection::getAccess() const {
  return this->access;
}

ReferenceType &ReferencedCollection::getReferenceType() const {
  auto &container_element_type =
      this->getAccess().getCollectionAccessed().getElementType();
  MEMOIR_ASSERT(
      (Type::is_reference_type(container_element_type)),
      "Attempt to construct ReferencedCollection for access to collection"
      " of non-reference element type");

  return static_cast<ReferenceType &>(container_element_type);
}

CollectionType &ReferencedCollection::getType() const {
  auto &referenced_type = this->getReferenceType().getReferencedType();
  MEMOIR_ASSERT((Type::is_collection_type(referenced_type)),
                "Attempt to construct ReferencedCollection for access to"
                " non-collection reference type");

  return static_cast<CollectionType &>(referenced_type);
}

Type &ReferencedCollection::getElementType() const {
  return this->getType().getElementType();
}

bool ReferencedCollection::operator==(const ReferencedCollection &other) const {
  return (&(this->getAccess()) != &(other.getAccess()));
}

std::string ReferencedCollection::toString(std::string indent) const {
  return "referenced collection";
}

/*
 * ControlPHI implementationn
 */
ControlPHICollection::ControlPHICollection(
    llvm::PHINode &phi_node,
    map<llvm::BasicBlock *, Collection *> &incoming)
  : phi_node(phi_node),
    incoming(incoming),
    Collection(CollectionKind::CONTROL_PHI) {
  // Do nothing.
}

Collection &ControlPHICollection::getIncomingCollection(unsigned idx) const {
  return this->getIncomingCollectionForBlock(this->getIncomingBlock(idx));
}

Collection &ControlPHICollection::getIncomingCollectionForBlock(
    llvm::BasicBlock &bb) const {
  auto found_incoming = this->incoming.find(&bb);
  if (found_incoming == this->incoming.end()) {
    MEMOIR_UNREACHABLE(
        "in ControlPHICollection::getIncomingCollectionForBlock"
        "basic block is not an incoming edge for this control PHI");
  }

  return *(found_incoming->second);
}

llvm::BasicBlock &ControlPHICollection::getIncomingBlock(unsigned idx) const {
  MEMOIR_ASSERT((idx < this->getNumIncoming()),
                "in ControlPHICollection::getIncomingBlock"
                "index out of range");

  auto bb = this->getPHI().getIncomingBlock(idx);
  MEMOIR_ASSERT((bb != nullptr),
                "in ControlPHICollection::getIncomingBlock"
                "no incoming edge at that index");

  return *bb;
}

unsigned ControlPHICollection::getNumIncoming() const {
  return this->getPHI().getNumIncomingValues();
}

CollectionType &ControlPHICollection::getType() const {
  return this->getIncomingCollection(0).getType();
}

Type &ControlPHICollection::getElementType() const {
  return this->getType().getElementType();
}

bool ControlPHICollection::operator==(const ControlPHICollection &other) const {
  return &(this->getPHI()) == &(other.getPHI());
}

std::string ControlPHICollection::toString(std::string indent) const {
  return "control PHI";
}

/*
 * RetPHI implementation
 */
RetPHICollection::RetPHICollection(
    llvm::CallBase &call,
    map<llvm::ReturnInst *, Collection *> &incoming)
  : call(call),
    incoming_returns(incoming_returns),
    incoming(incoming),
    Collection(CollectionKind::CALL_PHI) {
  /*
   * Initialize a partial ordering of the incoming returns.
   */
  for (auto it = incoming.begin(); it != incoming.end(); ++it) {
    auto incoming_ret = it->first;
    MEMOIR_NULL_CHECK(incoming_ret, "Incoming return is NULL!");
    this->incoming_returns.push_back(incoming_ret);
  }
}

Collection &RetPHICollection::getIncomingCollection(unsigned idx) const {
  return this->getIncomingCollectionForReturn(this->getIncomingReturn(idx));
}

Collection &RetPHICollection::getIncomingCollectionForReturn(
    llvm::ReturnInst &I) const {
  auto found_incoming = this->incoming.find(&I);

  MEMOIR_ASSERT((found_incoming != this->incoming.end()),
                "Could not find an incoming collection for the given return");

  return *(found_incoming->second);
}

llvm::ReturnInst &RetPHICollection::getIncomingReturn(unsigned idx) const {
  MEMOIR_ASSERT((idx < this->getNumIncoming()),
                "Attempt to get out-of-range incoming return");

  return *(this->incoming_returns.at(idx));
}

unsigned RetPHICollection::getNumIncoming() const {
  return this->incoming_returns.size();
}

CollectionType &RetPHICollection::getType() const {
  MEMOIR_ASSERT((this->getNumIncoming() > 0),
                "No incoming collections exist to determine the RetPHI's type");
  return this->getIncomingCollection(0).getType();
}

Type &RetPHICollection::getElementType() const {
  return this->getType().getElementType();
}

bool RetPHICollection::operator==(const RetPHICollection &other) const {
  return (&(this->getCall()) == &(other.getCall()));
}

std::string RetPHICollection::toString(std::string indent) const {
  return "call PHI";
}

/*
 * ArgPHI implementation
 */
ArgPHICollection::ArgPHICollection(
    llvm::Argument &argument,
    map<llvm::CallBase *, Collection *> &incoming)
  : argument(argument),
    incoming(incoming),
    Collection(CollectionKind::ARG_PHI) {
  /*
   * Initialize a partial ordering of the incoming calls.
   */
  for (auto it = incoming.begin(); it != incoming.end(); ++it) {
    this->incoming_calls.push_back(it->first);
  }
}

Collection &ArgPHICollection::getIncomingCollection(unsigned idx) const {
  return this->getIncomingCollectionForCall(this->getIncomingCall(idx));
}

Collection &ArgPHICollection::getIncomingCollectionForCall(
    llvm::CallBase &CB) const {
  auto found_incoming = this->incoming.find(&CB);

  MEMOIR_ASSERT((found_incoming != this->incoming.end()),
                "Could not find an incoming collection for the given call!");

  return *(found_incoming->second);
}

llvm::CallBase &ArgPHICollection::getIncomingCall(unsigned idx) const {
  MEMOIR_ASSERT((idx < this->getNumIncoming()),
                "Attempt to get incoming call for index out of range");

  return *(this->incoming_calls.at(idx));
}

unsigned ArgPHICollection::getNumIncoming() const {
  return this->incoming_calls.size();
}

llvm::Argument &ArgPHICollection::getArgument() const {
  return this->argument;
}

CollectionType &ArgPHICollection::getType() const {
  auto llvm_func = this->getArgument().getParent();
  MEMOIR_NULL_CHECK(
      llvm_func,
      "Attempt to get the type of an argument of the NULL function");

  auto &memoir_func = MemOIRFunction::get(*llvm_func);
  auto &memoir_func_type = memoir_func.getFunctionType();
  auto arg_index = this->getArgument().getArgNo();
  auto memoir_arg_type = memoir_func_type.getParamType(arg_index);
  MEMOIR_NULL_CHECK(
      memoir_arg_type,
      "Attempt to get CollectionType of a non-MemOIR typed argument");

  MEMOIR_ASSERT(
      (Type::is_collection_type(*memoir_arg_type)),
      "Attempt to get CollectionType of a non-collection MemOIR Type");

  return static_cast<CollectionType &>(*memoir_arg_type);
}

Type &ArgPHICollection::getElementType() const {
  return this->getType().getElementType();
}

bool ArgPHICollection::operator==(const ArgPHICollection &other) const {
  return (&(this->getArgument()) == &(other.getArgument()));
}

std::string ArgPHICollection::toString(std::string indent) const {
  return "arg PHI collection";
}

/*
 * DefPHI implementation
 */
DefPHICollection::DefPHICollection(WriteInst &access)
  : access(access),
    Collection(CollectionKind::DEF_PHI) {
  // Do nothing.
}

Collection &DefPHICollection::getCollection() const {
  return this->getAccess().getCollectionAccessed();
}

WriteInst &DefPHICollection::getAccess() const {
  return this->access;
}

CollectionType &DefPHICollection::getType() const {
  return this->getCollection().getType();
}

Type &DefPHICollection::getElementType() const {
  return this->getCollection().getElementType();
}

bool DefPHICollection::operator==(const DefPHICollection &other) const {
  return (&(this->getCollection()) == &(other.getCollection()))
         && (&(this->getAccess()) == &(other.getAccess()));
}

std::string DefPHICollection::toString(std::string indent) const {
  return "def PHI";
}

/*
 * UsePHI implementation
 */
UsePHICollection::UsePHICollection(ReadInst &access)
  : access(access),
    Collection(CollectionKind::USE_PHI) {
  // Do nothing.
}

Collection &UsePHICollection::getCollection() const {
  return this->getAccess().getCollectionAccessed();
}

ReadInst &UsePHICollection::getAccess() const {
  return this->access;
}

CollectionType &UsePHICollection::getType() const {
  return this->getCollection().getType();
}

Type &UsePHICollection::getElementType() const {
  return this->getCollection().getElementType();
}

bool UsePHICollection::operator==(const UsePHICollection &other) const {
  return (&(this->getCollection()) == &(other.getCollection()))
         && (&(this->getAccess()) == &(other.getAccess()));
}

std::string UsePHICollection::toString(std::string indent) const {
  return "use PHI";
}

/*
 * JoinPHI implementation
 */
JoinPHICollection::JoinPHICollection(JoinInst &join_inst)
  : join_inst(join_inst),
    Collection(CollectionKind::JOIN_PHI) {
  // Do nothing.
}

JoinInst &JoinPHICollection::getJoin() const {
  return this->join_inst;
}

unsigned JoinPHICollection::getNumberOfJoinedCollections() const {
  return this->getJoin().getNumberOfJoins();
}

Collection &JoinPHICollection::getJoinedCollection(unsigned join_index) const {
  MEMOIR_ASSERT((join_index < this->getNumberOfJoinedCollections()),
                "Attempt to get collection being joined out of range.");
  return this->getJoin().getJoinedCollection(join_index);
}

CollectionType &JoinPHICollection::getType() const {
  MEMOIR_ASSERT((this->getNumberOfJoinedCollections() > 0),
                "Attempt to get type of join PHI with no arguments");
  return this->getJoinedCollection(0).getType();
}

Type &JoinPHICollection::getElementType() const {
  return this->getType().getElementType();
}

bool JoinPHICollection::operator==(const JoinPHICollection &other) const {
  return (&(this->getJoin()) == &(other.getJoin()));
}

std::string JoinPHICollection::toString(std::string indent) const {
  return "join PHI";
}

/*
 * SliceCollection implementation
 */
SliceCollection::SliceCollection(SliceInst &slice_inst)
  : slice_inst(slice_inst),
    Collection(CollectionKind::SLICE) {
  // Do nothing.
}

SliceInst &SliceCollection::getSlice() const {
  return this->slice_inst;
}

Collection &SliceCollection::getSlicedCollection() const {
  return this->getSlice().getCollection();
}

CollectionType &SliceCollection::getType() const {
  return this->getSlicedCollection().getType();
}

Type &SliceCollection::getElementType() const {
  return this->getType().getElementType();
}

bool SliceCollection::operator==(const SliceCollection &other) const {
  return (&(this->getSlice()) == &(other.getSlice()));
}

std::string SliceCollection::toString(std::string indent) const {
  return "slice collection";
}

} // namespace llvm::memoir
