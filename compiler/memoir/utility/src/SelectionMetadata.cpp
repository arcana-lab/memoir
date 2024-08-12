#include "llvm/IR/Constants.h"

#include "memoir/utility/Metadata.hpp"
#include "memoir/utility/MetadataUtils.hpp"

#include "memoir/support/Casting.hpp"

namespace llvm::memoir {

OPERAND(SelectionMetadata, Implementation, 0)

std::string SelectionMetadata::getImplementation() const {
  // Fetch the metadata.
  auto &metadata = this->getImplementationMD();
  auto &constant_as_metadata = MEMOIR_SANITIZE(
      dyn_cast<llvm::ConstantAsMetadata>(&metadata),
      "Malformed LiveOutMetadata, expected an llvm::ConstantAsMetadata");

  // Unpack the selection id from the metadata.
  auto *constant = constant_as_metadata.getValue();
  auto &constant_as_data_array = MEMOIR_SANITIZE(
      dyn_cast_or_null<llvm::ConstantDataArray>(constant),
      "Malformed LiveOutMetadata, expected an llvm::ConstantDataArray");

  return constant_as_data_array.getAsString().str();
}

void SelectionMetadata::setImplementation(std::string id) {
  // Fetch relevant information.
  auto &metadata = this->getMetadata();
  auto &context = metadata.getContext();

  // Create a constant for the argument number.
  auto *selection_constant =
      llvm::ConstantDataArray::getString(context, llvm::StringRef(id));

  // Create a metadata wrapper.
  auto *selection_metadata = llvm::ConstantAsMetadata::get(selection_constant);

  // Append the metadata.
  if (metadata.getNumOperands() == 1) {
    metadata.pop_back();
  }
  metadata.push_back(selection_metadata);
}

} // namespace llvm::memoir
