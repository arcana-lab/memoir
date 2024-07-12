#include "llvm/IR/Constants.h"

#include "memoir/utility/Metadata.hpp"
#include "memoir/utility/MetadataUtils.hpp"

#include "memoir/support/Casting.hpp"

namespace llvm::memoir {

unsigned LiveOutMetadata::getArgNo() const {
  // Fetch the metadata.
  auto &metadata = this->getArgNoMD();
  auto &constant_as_metadata = MEMOIR_SANITIZE(
      dyn_cast<llvm::ConstantAsMetadata>(&metadata),
      "Malformed LiveOutMetadata, expected an llvm::ConstantAsMetadata");

  // Unpack the argument number from the metadata.
  auto *constant = constant_as_metadata.getValue();
  auto &constant_int = MEMOIR_SANITIZE(
      dyn_cast_or_null<llvm::ConstantInt>(constant),
      "Malformed LiveOutMetadata, expected an llvm::ConstantInt");
  auto arg_number = unsigned(constant_int.getZExtValue());

  return arg_number;
}

void LiveOutMetadata::setArgNo(unsigned arg_number) {
  // Fetch relevant information.
  auto &metadata = this->getMetadata();
  auto &context = metadata.getContext();

  // Create a constant for the argument number.
  auto *unsigned_type = llvm::IntegerType::get(context, 8);
  auto *arg_number_constant = llvm::ConstantInt::get(unsigned_type, arg_number);

  // Create a metadata wrapper.
  auto *arg_number_metadata =
      llvm::ConstantAsMetadata::get(arg_number_constant);

  // Append the metadata.
  if (metadata.getNumOperands() == 1) {
    metadata.pop_back();
  }
  metadata.push_back(arg_number_metadata);
}

OPERAND(LiveOutMetadata, ArgNo, 0)

} // namespace llvm::memoir
