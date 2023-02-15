#include "memoir/ir/Structs.hpp"
#include "memoir/ir/Instructions.hpp"

/*
 * This file contains a substrate for analyzing MemOIR structs.
 *
 * Author(s): Tommy McMichen
 * Created: December 19, 2022
 */

namespace llvm::memoir {

/*
 * Struct implementation
 */
Struct::Struct(StructCode code) : code(code) {
  // Do nothing.
}

StructCode Struct::getCode() const {
  return this->code;
}

std::ostream &operator<<(std::ostream &os, const Struct &S) {
  os << S.toString("");
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Struct &S) {
  os << S.toString("");
  return os;
}

/*
 * BaseStruct implementation
 */
BaseStruct::BaseStruct(StructAllocInst &allocation)
  : allocation(allocation),
    Struct(StructCode::BASE) {
  // Do nothing.
}

StructAllocInst &BaseStruct::getAllocInst() const {
  return this->allocation;
}

StructType &BaseStruct::getType() const {
  return this->getAllocInst().getStructType();
}

std::string BaseStruct::toString(std::string indent) const {
  std::string str;

  str = "(base struct\n";
  str += indent + this->getAllocInst().toString(indent) + "\n";
  str += indent + ")\n";

  return str;
}

/*
 * ContainedStruct implementation
 */

ContainedStruct::ContainedStruct(GetInst &access_to_container, StructCode code)
  : access(access_to_container),
    Struct(code) {
  // Do nothing.
}

ContainedStruct::ContainedStruct(GetInst &access_to_container)
  : ContainedStruct(access_to_container, StructCode::CONTAINED) {
  // Do nothing.
}

GetInst &ContainedStruct::getAccess() const {
  return this->access;
}

Collection &ContainedStruct::getContainer() const {
  return this->getAccess().getCollectionAccessed();
}

StructType &ContainedStruct::getType() const {
  auto &container_element_type = this->getContainer().getElementType();
  MEMOIR_ASSERT(Type::is_struct_type(container_element_type),
                "Attempt to construct a ContainedStruct for a container"
                "of non-struct element type");

  return static_cast<StructType &>(container_element_type);
}

std::string ContainedStruct::toString(std::string indent) const {
  std::string str;

  str = "(contained struct\n";
  str += indent + "  container: \n";
  str +=
      indent + "    " + this->getContainer().toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}

/*
 * NestedStruct implementation
 */
NestedStruct::NestedStruct(StructGetInst &access)
  : ContainedStruct(access, StructCode::NESTED) {
  // Do nothing.
}

StructGetInst &NestedStruct::getStructAccess() const {
  auto &struct_get_inst = static_cast<StructGetInst &>(this->getAccess());
  return struct_get_inst;
}

Struct &NestedStruct::getNestingStruct() const {
  return this->getStructAccess().getStructAccessed();
}

Collection &NestedStruct::getNestingCollection() const {
  return this->getStructAccess().getCollectionAccessed();
}

std::string NestedStruct::toString(std::string indent) const {
  std::string str;

  str = "(nested struct\n";
  str += indent + "  nesting collection: \n";
  str += indent + "    "
         + this->getNestingCollection().toString(indent + "    ") + "\n";
  str += indent + "  nesting struct: \n";
  str += indent + "    " + this->getNestingStruct().toString(indent + "    ")
         + "\n";
  str += indent + ")";

  return str;
}

/*
 * ReferencedStruct implementation
 */
ReferencedStruct::ReferencedStruct(ReadInst &access)
  : access(access),
    Struct(StructCode::REFERENCED) {
  // Do nothing.
}

ReadInst &ReferencedStruct::getAccess() const {
  return this->access;
}

ReferenceType &ReferencedStruct::getReferenceType() const {
  auto &access_type =
      this->getAccess().getCollectionAccessed().getElementType();
  MEMOIR_ASSERT(Type::is_reference_type(access_type),
                "access is not to a reference type");

  return static_cast<ReferenceType &>(access_type);
}

StructType &ReferencedStruct::getType() const {
  auto &referenced_type = this->getReferenceType().getReferencedType();
  MEMOIR_ASSERT(Type::is_struct_type(referenced_type),
                "Attempt to get struct type of a non-struct referenced type");

  return static_cast<StructType &>(referenced_type);
}

std::string ReferencedStruct::toString(std::string indent) const {
  std::string str;

  // TODO: flesh out print info

  str += "(referenced struct)";

  return str;
}

/*
 * ControlPHIStruct implementation
 */
ControlPHIStruct::ControlPHIStruct(llvm::PHINode &phi_node,
                                   map<llvm::BasicBlock *, Struct *> &incoming)
  : phi_node(phi_node),
    incoming(incoming),
    Struct(StructCode::CONTROL_PHI) {
  // Do nothing.
}

Struct &ControlPHIStruct::getIncomingStruct(unsigned idx) const {
  auto &incoming_bb = this->getIncomingBlock(idx);
  return this->getIncomingStructForBlock(incoming_bb);
}

Struct &ControlPHIStruct::getIncomingStructForBlock(
    llvm::BasicBlock &BB) const {
  auto found_struct = this->incoming.find(&BB);
  MEMOIR_ASSERT(
      (found_struct != this->incoming.end()),
      "couldn't find an incoming struct summary for the given basic block");

  return *(found_struct->second);
}

llvm::BasicBlock &ControlPHIStruct::getIncomingBlock(unsigned idx) const {
  auto bb = this->phi_node.getIncomingBlock(idx);
  MEMOIR_NULL_CHECK(bb, "index is out of range");

  return *bb;
}

unsigned ControlPHIStruct::getNumIncoming() const {
  return this->phi_node.getNumIncomingValues();
}

llvm::PHINode &ControlPHIStruct::getPHI() const {
  return this->phi_node;
}

StructType &ControlPHIStruct::getType() const {
  MEMOIR_ASSERT((this->getNumIncoming() > 0),
                "no incoming structs for type information");

  for (auto i = 0; i < this->getNumIncoming(); i++) {
    auto &incoming_struct = this->getIncomingStruct(i);
    if (this != &incoming_struct) {
      return incoming_struct.getType();
    }
  }

  MEMOIR_UNREACHABLE("Unable to get type information for control PHI because"
                     " all incoming edges are the control PHI");
}

std::string ControlPHIStruct::toString(std::string indent) const {
  std::string str;

  // TODO: flesh out print info

  str += "(control phi struct)";

  return str;
}

/*
 * ArgPHIStruct implementation
 */
ArgPHIStruct::ArgPHIStruct(llvm::Argument &argument,
                           vector<llvm::CallBase *> &incoming_calls,
                           map<llvm::CallBase *, Struct *> &incoming)
  : argument(argument),
    incoming_calls(incoming_calls),
    incoming(incoming),
    Struct(StructCode::ARGUMENT_PHI) {
  // Do nothing.
}

Struct &ArgPHIStruct::getIncomingStruct(uint64_t idx) const {
  auto &incoming_call = this->getIncomingCall(idx);
  return this->getIncomingStructForCall(incoming_call);
}

Struct &ArgPHIStruct::getIncomingStructForCall(llvm::CallBase &CB) const {
  auto found_struct = this->incoming.find(&CB);
  MEMOIR_ASSERT((found_struct != this->incoming.end()),
                "no incoming struct for given call");
  return *(found_struct->second);
}

llvm::CallBase &ArgPHIStruct::getIncomingCall(uint64_t idx) const {
  MEMOIR_ASSERT((idx < this->getNumIncoming()), "index out of range");

  return *(this->incoming_calls.at(idx));
}

uint64_t ArgPHIStruct::getNumIncoming() const {
  return this->incoming_calls.size();
}

StructType &ArgPHIStruct::getType() const {
  MEMOIR_ASSERT((this->getNumIncoming() > 0),
                "no incoming structs for type information");

  for (auto i = 0; i < this->getNumIncoming(); i++) {
    auto &incoming_struct = this->getIncomingStruct(i);
    if (this != &incoming_struct) {
      return incoming_struct.getType();
    }
  }

  MEMOIR_UNREACHABLE("Unable to get type information for argument PHI because"
                     " all incoming edges are the argument PHI");
}

std::string ArgPHIStruct::toString(std::string indent) const {
  std::string str;

  // TODO: flesh out print info

  str += "(argument phi struct)";

  return str;
}

/*
 * RetPHIStruct implementation
 */
RetPHIStruct::RetPHIStruct(llvm::CallBase &call,
                           vector<llvm::ReturnInst *> &incoming_returns,
                           map<llvm::ReturnInst *, Struct *> &incoming)
  : call(call),
    incoming_returns(incoming_returns),
    incoming(incoming),
    Struct(StructCode::RETURN_PHI) {
  // Do nothing.
}

Struct &RetPHIStruct::getIncomingStruct(uint64_t idx) const {
  auto &return_inst = this->getIncomingReturn(idx);
  return this->getIncomingStructForReturn(return_inst);
}

Struct &RetPHIStruct::getIncomingStructForReturn(llvm::ReturnInst &RI) const {
  auto found_struct = this->incoming.find(&RI);
  MEMOIR_ASSERT(
      (found_struct != this->incoming.end()),
      "couldn't find an incoming struct summary for the given return instruction");

  return *(found_struct->second);
}

llvm::ReturnInst &RetPHIStruct::getIncomingReturn(uint64_t idx) const {
  MEMOIR_ASSERT((idx < this->getNumIncoming()), "index out of range");

  return *(this->incoming_returns.at(idx));
}

uint64_t RetPHIStruct::getNumIncoming() const {
  return this->incoming_returns.size();
}

llvm::CallBase &RetPHIStruct::getCall() const {
  return this->call;
}

StructType &RetPHIStruct::getType() const {
  MEMOIR_ASSERT((this->getNumIncoming() > 0),
                "no incoming structs for type information");

  for (auto i = 0; i < this->getNumIncoming(); i++) {
    auto &incoming_struct = this->getIncomingStruct(i);
    if (this != &incoming_struct) {
      return incoming_struct.getType();
    }
  }

  MEMOIR_UNREACHABLE("Unable to get type information for return PHI because"
                     " all incoming edges are the return PHI");
}

std::string RetPHIStruct::toString(std::string indent) const {
  std::string str;

  // TODO: flesh out print info

  str += "(return phi struct)";

  return str;
}

} // namespace llvm::memoir
