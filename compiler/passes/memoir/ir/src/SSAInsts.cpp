#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InstructionUtils.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

/*
 * UsePHIInst implementation
 */
RESULTANT(UsePHIInst, ResultCollection)
OPERAND(UsePHIInst, UsedCollection, 0)

llvm::Instruction &UsePHIInst::getUseInst() const {
  // Look for metadata attached to this use.
  auto *value = MetadataManager::getMetadata(this->getCallInst(),
                                             MetadataType::MD_USE_PHI);
  auto *inst = dyn_cast_or_null<llvm::Instruction>(value);
  MEMOIR_NULL_CHECK(inst, "Couldn't get the UseInst");
  return *inst;
}

void UsePHIInst::setUseInst(llvm::Instruction &I) const {
  // Update this information in the MetadataManager
  MetadataManager::setMetadata(this->getCallInst(),
                               MetadataType::MD_USE_PHI,
                               &I);
}

void UsePHIInst::setUseInst(MemOIRInst &I) const {
  this->setUseInst(I.getCallInst());
}

TO_STRING(UsePHIInst)

// DefPHIInst implementation
RESULTANT(DefPHIInst, ResultCollection)
OPERAND(DefPHIInst, DefinedCollection, 0)

llvm::Instruction &DefPHIInst::getDefInst() const {
  // Look for metadata attached to this definition.
  auto *value = MetadataManager::getMetadata(this->getCallInst(),
                                             MetadataType::MD_DEF_PHI);
  auto *inst = dyn_cast_or_null<llvm::Instruction>(value);
  MEMOIR_NULL_CHECK(inst, "Couldn't get the DefInst");
  return *inst;
}

void DefPHIInst::setDefInst(llvm::Instruction &I) const {
  // Update this information in the MetadataManager
  MetadataManager::setMetadata(this->getCallInst(),
                               MetadataType::MD_DEF_PHI,
                               &I);
}

void DefPHIInst::setDefInst(MemOIRInst &I) const {
  this->setDefInst(I.getCallInst());
}

TO_STRING(DefPHIInst)

// ArgPHIInst implementation
RESULTANT(ArgPHIInst, ResultCollection)
OPERAND(ArgPHIInst, InputCollection, 0)
// TODO: implement metadata for storing the incoming collections.
TO_STRING(ArgPHIInst)

// RetPHIInst implementation
RESULTANT(RetPHIInst, ResultCollection)
OPERAND(RetPHIInst, InputCollection, 0)
// TODO: implement metadata for storing the incoming collections.
TO_STRING(RetPHIInst)

} // namespace llvm::memoir
