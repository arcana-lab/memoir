#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

/*
 * StructSummary implementation
 */
StructSummary::StructSummary(StructCode code) : code(code) {
  // Do nothing.
}

StructCode StructSummary::getCode() const {
  return this->code;
}

/*
 * BaseStructSummary implementation
 */
BaseStructSummary::BaseStructSummary(AllocationSummary &allocation)
  : allocation(allocation),
    StructSummary(StructCode::BASE) {
  // Do nothing.
}

AllocationSummary &BaseStructSummary::getAllocation() const {
  return this->allocation;
}

TypeSummary &BaseStructSummary::getType() const {
  return this->getAllocation().getType();
}

/*
 * NestedStructSummary implementation
 */
NestedStructSummary::NestedStructSummary(FieldSummary &field)
  : field(field),
    StructSummary(StructCode::NESTED) {
  // Do nothing.
}

StructSummary &NestedStructSummary::getContainer() const {
  return this->struct_container;
}

TypeSummary &NestedStructSummary::getType() const {
  return this->getContainer().getType().getField(this->field_index);
}

/*
 * ReferencedStructSummary implementation
 */
ReferencedStructSummary::ReferencedStructSummary(ReadSummary &access)
  : access(access),
    StructSummary(StructCode::REFERENCED) {
  // Do nothing.
}

ReadSummary &ReferencedStructSummary::getAccess() const {
  return this->access;
}

llvm::CallInst &ReferencedStructSummary::getCallInst() const {
  return this->getAccess().getCallInst();
}

ReferenceTypeSummary &ReferencedStructSummary::getReferenceType() const {
  auto &access_type = this->getAccess().getType();
  assert((access_type.getCode() == TypeCode::ReferenceTy)
         && "in ReferencedTypeSummary::getReferenceType"
            "access is not to a reference type");
  return static_cast<ReferenceTypeSummary &>(access_type);
}

TypeSummary &ReferencedStructSummary::getType() const {
  return this->getReferenceType().getReferencedType();
}

/*
 * ControlPHIStructSummary implementation
 */
ControlPHIStructSummary::ControlPHIStructSummary(
    llvm::PHINode &phi_node,
    map<llvm::BasicBlock *, StructSummary *> &incoming)
  : phi_node(phi_node),
    incoming(incoming),
    StructSummary(StructCode::CONTROL_PHI) {
  // Do nothing.
}

StructSummary &ControlPHIStructSummary::getIncomingStruct(unsigned idx) const {
  auto &incoming_bb = this->getIncomingBlock(idx);
  return this->getIncomingStructForBlock(incoming_bb);
}

StructSummary &ControlPHIStructSummary::getIncomingStructForBlock(
    const llvm::BasicBlock &BB) const {
  auto found_struct = this->incoming.find(&BB);
  assert(
      (found_struct != this->incoming.end())
      && "in StructSummary::getIncomingStructForBlock"
         "couldn't find an incoming struct summary for the given basic block");

  return *(found_struct->second);
}

llvm::BasicBlock &ControlPHIStructSummary::getIncomingBlock(
    unsigned idx) const {
  auto bb = this->phi_node.getIncomingBlock(idx);
  assert(bb != nullptr
         && "in ControlPHIStructSummary::getIncomingBlock"
            "index is out of range");
  return *bb;
}

llvm::PHINode &ControlPHIStructSummary::getPHI() const {
  return this->phi_node;
}

TypeSummary &ControlPHIStructSummary::getType() const {
  assert((this->getNumIncoming() > 0)
         && "in ControlPHIStructSummary"
            "no incoming structs for type information");
  auto &first_struct = this->getIncomingStruct(0).getType();
  return first_struct.getType();
}

/*
 * CallPHIStructSummary implementation
 */
CallPHIStructSummary::CallPHIStructSummary(
    llvm::Argument &argument,
    vector<llvm::CallBase *> &incoming_calls,
    map<llvm::CallBase *, StructSummary *> &incoming)
  : argument(argument),
    incoming_calls(incoming_calls),
    incoming(incoming),
    StructSummary(StructCode::CALL_PHI) {
  // Do nothing.
}

StructSummary &CallPHIStructSummary::getIncomingStruct(uint64_t idx) const {
  auto &incoming_call = this->getIncomingCall(idx);
  return this->getIncomingStructForCall(incoming_call);
}

StructSummary &CallPHIStructSummary::getIncomingStructForCall(
    const llvm::CallBase &call) const {
  auto found_struct = this->incoming.find(&call);
  assert((found_struct != this->incoming.end())
         && "in CallPHIStructSummary::getIncomingStructForCall"
            "no incoming struct for given call");
  return *(found_struct->second);
}

llvm::CallBase &CallPHIStructSummary::getIncomingCall(uint64 idx) const {
  assert((idx < this->getNumIncoming())
         && "in CallPHIStructSummary::getIncomingCall"
            "index out of range");
  return *(this->incoming_calls.at(idx));
}

uint64_t CallPHIStructSUmmary::getNumIncoming() const {
  return this->incoming_calls.size();
}

TypeSummary &CallPHIStructSummary::getType() const {
  assert((this->getNumIncoming() > 0)
         && "CallPHIStructSummary::getType"
            "no incoming structs for type information");
  auto &first_struct = this->getIncomingStruct(0);
  return first_struct.getType();
}

} // namespace llvm::memoir
