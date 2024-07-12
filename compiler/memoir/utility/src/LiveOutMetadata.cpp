#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Casting.hpp"

namespace llvm::memoir {

unsigned LiveOutMetadata::getNumLiveOuts() const {
  auto num_operands = this->getMetadata().getNumOperands();
  MEMOIR_ASSERT(num_operands % 2 == 0, "Malformed LiveOutMetadata!");
  return num_operands / 2;
}

llvm::Value &LiveOutMetadata::getArgument(unsigned i) const {
  auto &metadata = this->getArgumentMD(i);
  auto &value_as_metadata = MEMOIR_SANITIZE(
      dyn_cast<llvm::ValueAsMetadata>(&metadata),
      "Argument operand of LiveOutMetadata is not a ValueAsMetadata");
  auto &value =
      MEMOIR_SANITIZE(value_as_metadata.getValue(),
                      "Argument operand of LiveOutMetadata is a NULL value!");
  return value;
}

llvm::Metadata &LiveOutMetadata::getArgumentMD(unsigned i) const {
  return *(this->getArgumentMDOperand(i).get());
}

const llvm::MDOperand &LiveOutMetadata::getArgumentMDOperand(unsigned i) const {
  return this->getMetadata().getOperand(2 * i);
}

llvm::Value &LiveOutMetadata::getLiveOut(unsigned i) const {
  auto &metadata = this->getLiveOutMD(i);
  auto &value_as_metadata = MEMOIR_SANITIZE(
      dyn_cast<llvm::ValueAsMetadata>(&metadata),
      "LiveOut operand of LiveOutMetadata is not a ValueAsMetadata");
  auto &value =
      MEMOIR_SANITIZE(value_as_metadata.getValue(),
                      "LiveOut operand of LiveOutMetadata is a NULL value!");
  return value;
}

llvm::Metadata &LiveOutMetadata::getLiveOutMD(unsigned i) const {
  return *(this->getLiveOutMDOperand(i).get());
}

const llvm::MDOperand &LiveOutMetadata::getLiveOutMDOperand(unsigned i) const {
  return this->getMetadata().getOperand(2 * i);
}

void LiveOutMetadata::addLiveOut(llvm::Value &argument,
                                 llvm::Value &live_out) const {
  // Construct the values as metadata.
  auto *argument_metadata = llvm::ValueAsMetadata::get(&argument);
  auto *live_out_metadata = llvm::ValueAsMetadata::get(&live_out);

  // Append to the MDTuple.
  auto &md_tuple = this->getMetadata();
  md_tuple.push_back(argument_metadata);
  md_tuple.push_back(live_out_metadata);

  return;
}

} // namespace llvm::memoir
