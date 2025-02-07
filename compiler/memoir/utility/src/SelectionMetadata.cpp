#include "llvm/IR/Constants.h"

#include "memoir/utility/Metadata.hpp"
#include "memoir/utility/MetadataUtils.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

VAR_OPERAND(SelectionMetadata, Implementation, 0)

std::optional<std::string> SelectionMetadata::getImplementation(
    unsigned i) const {
  if (this->getMetadata().getNumOperands() <= i) {
    return {};
  }

  auto &operand = this->getImplementationMDOperand(i);
  auto *selection = operand.get();
  if (auto *selection_node = dyn_cast<llvm::MDNode>(selection)) {
    if (selection_node->getNumOperands() == 0) {
      return {};
    }
  }

  return Metadata::to_string(*selection);
}

llvm::iterator_range<SelectionMetadata::iterator> SelectionMetadata::
    implementations() {
  return llvm::make_range(this->impl_begin(), this->impl_end());
}

SelectionMetadata::iterator SelectionMetadata::impl_begin() {
  return iterator(this->getMetadata().op_begin());
}

SelectionMetadata::iterator SelectionMetadata::impl_end() {
  return iterator(this->getMetadata().op_end());
}

void SelectionMetadata::setImplementation(std::string id, unsigned i) {
  // Fetch relevant information.
  auto &metadata = this->getMetadata();
  auto &context = metadata.getContext();

  // Create a constant for the argument number.
  auto *selection_constant =
      llvm::ConstantDataArray::getString(context,
                                         id,
                                         /* AddNull = */ false);

  // Create a metadata wrapper.
  auto *selection_metadata = llvm::ConstantAsMetadata::get(selection_constant);

  // Pad the MDTuple with NULL.
  while (metadata.getNumOperands() <= i) {
    metadata.push_back(MDNode::get(context, {}));
  }

  // Set the selection metadata.
  metadata.replaceOperandWith(i, selection_metadata);
}

} // namespace llvm::memoir
