#include "llvm/IR/Constants.h"

#include "memoir/utility/Metadata.hpp"
#include "memoir/utility/MetadataUtils.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

VAR_OPERAND(SelectionMetadata, Implementation, 0)

std::string SelectionMetadata::getImplementation(unsigned i) const {
  return Metadata::to_string(this->getImplementationMD(i));
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

  // Append the metadata.
  if (metadata.getNumOperands() == 1) {
    metadata.pop_back();
  }
  metadata.push_back(selection_metadata);
}

} // namespace llvm::memoir
