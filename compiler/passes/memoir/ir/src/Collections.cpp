#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * Collection implementation
 */
Collection::Collection(CollectionCode code) : code(code) {
  // Do nothing.
}

CollectionCode Collection::getCode() const {
  return this->code;
}

bool Collection::operator==(const Collection &other) const {
  if (this->code != other.code) {
    return false;
  }

  switch (this->getCode()) {
    case CollectionCode::BASE:
      return (static_cast<const BaseCollection &>(*this)
              == static_cast<const BaseCollection &>(other));
    case CollectionCode::FIELD_ARRAY:
      return (static_cast<const FieldArray &>(*this)
              == static_cast<const FieldArray &>(other));
    case CollectionCode::NESTED:
      return (static_cast<const ControlPHICollection &>(*this)
              == static_cast<const ControlPHICollection &>(other));
    case CollectionCode::CONTROL_PHI:
      return (static_cast<const ControlPHICollection &>(*this)
              == static_cast<const ControlPHICollection &>(other));
    case CollectionCode::CALL_PHI:
      return (static_cast<const CallPHICollection &>(*this)
              == static_cast<const CallPHICollection &>(other));
    case CollectionCode::DEF_PHI:
      return (static_cast<const DefPHICollection &>(*this)
              == static_cast<const DefPHICollection &>(other));
    case CollectionCode::USE_PHI:
      return (static_cast<const UsePHICollection &>(*this)
              == static_cast<const UsePHICollection &>(other));
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
    Collection(CollectionCode::BASE) {
  // Do nothing.
}

CollectionAllocInst &BaseCollection::getAllocation() const {
  return this->allocation;
}

CollectionType &BaseCollection::getType() const {
  return this->getAllocation().getType();
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
    Collection(CollectionCode::FIELD_ARRAY) {
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
NestedCollection::NestedCollection(GetInst &get_inst)
  : get_inst(get_inst),
    Collection(CollectionCode::NESTED) {
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
  MEMOIR_ASSERT((Type::is_collection_type(nested_element_type)),
                "Attempt to construct NestedCollection for access to collection"
                "of non-collection element type");

  auto &nesting_collection_type = static_cast<CollectionType &>(nesting_type);
  return nesting_collection_type.getElementType();
}

Type &NestedCollection::getElementType() const {
  return this->getType().getElementType();
}

/*
 * ControlPHI implementationn
 */
ControlPHICollection::ControlPHICollection(
    llvm::PHINode *phi_node,
    map<llvm::BasicBlock *, Collection *> &incoming)
  : phi_node(phi_node),
    incoming(incoming),
    Collection(CollectionCode::CONTROL_PHI) {
  // Do nothing.
}

Collection &ControlPHICollection::getIncomingCollection(unsigned idx) const {
  auto &bb = this->getIncomingBlock(idx);

  return this->getIncomingCollectionForBlock(bb);
}

Collection &ControlPHICollection::getIncomingCollectionForBlock(
    const llvm::BasicBlock &bb) {
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
  return this->getPHI().getNumIncomingValue();
}

CollectionType &ControlPHICollection::getType() const {
  return this->getIncomingCollection(0).getType();
}

Type &ControlPHICollection::getElementType() const {
  return this->getType().getElementType();
}

bool ControlPHICollection::operator==(const ControlPHI &other) const {
  return &(this->getPHI()) == &(other.getPHI());
}

std::string ControlPHICollection::toString(std::string indent) const {
  return "control PHI";
}

/*
 * CallPHI implementation
 */
CallPHICollection::CallPHICollection(
    llvm::Argument &argument,
    map<llvm::CallBase *, Collection *> &incoming)
  : argument(argument),
    incoming(incoming),
    Collection(CollectionCode::CALL_PHI) {
  // Do nothing.
}

Collection &CallPHICollection::getIncomingCollection(unsigned idx) const {
  return this->getIncomingCollectionForCall(this->getIncomingCall(idx));
}

Collection &CallPHICollection::getIncomingCollectionForCall(
    const llvm::CallBase &CB) const {
  auto found_incoming = this->incoming.find(&CB);

  MEMOIR_ASSERT((found_incoming != this->incoming.end()),
                "Could not find an incoming collection for the given call!");

  return *(found_incoming->second);
}

llvm::CallBase &CallPHICollection::getIncomingCall(unsigned idx) const {
  MEMOIR_ASSERT((idx < this->getNumIncoming()),
                "Attempt to get incoming call for index out of range");

  return *(this->incoming_calls.at(idx));
}

unsigned CallPHICollection::getNumIncoming() const {
  return this->incoming_calls.size();
}

llvm::Argument &CallPHICollection::getArgument() const {
  return this->argument;
}

CollectionType &CallPHICollection::getType() const {
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

  return *memoir_arg_type;
}

Type &CallPHICollection::getElementType() const {
  return this->getType().getElementType();
}

/*
 * DefPHI implementation
 */
DefPHICollection::DefPHICollection(WriteInst &access)
  : access(access),
    Collection(CollectionCode::DEF_PHI) {
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
    Collection(CollectionCode::USE_PHI) {
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

bool UsePHICollection::operator==(const UsePHI &other) const {
  return (this->getCollection() == other.getCollection())
         && (this->getAccess() == other.getAccess());
}

std::string UsePHICollection::toString(std::string indent) const {
  return "use PHI";
}

/*
 * JoinPHI implementation
 */
JoinPHICollection::JoinPHICollection(JoinInst &join_inst)
  : join_inst(join_inst),
    Collection(CollectionCode::JOIN_PHI) {
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
  MEMOIR_ASSERT((this->getNumberOfJoinedCollections().size() > 0),
                "Attempt to get type of join PHI with no arguments");
  return this->getJoinedCollection(0).getType();
}

Type &JoinPHICollection::getElementType() const {
  return this->getType().getElementType();
}

} // namespace llvm::memoir
