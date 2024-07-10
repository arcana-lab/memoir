#include "llvm/IR/Constants.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Property.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

Property PropertyInst::getProperty() const {
  return Property(*this);
}

std::string PropertyInst::getPropertyID() const {
  auto &property_id_value = this->getPropertyIDOperand();

  GlobalVariable *property_id_global = nullptr;
  if (auto property_id_gep =
          dyn_cast<llvm::GetElementPtrInst>(&property_id_value)) {
    auto property_id_ptr = property_id_gep->getPointerOperand();
    property_id_global = dyn_cast<GlobalVariable>(property_id_ptr);
  } else if (auto property_id_const_gep =
                 dyn_cast<llvm::ConstantExpr>(&property_id_value)) {
    auto property_id_ptr = property_id_const_gep->getOperand(0);
    property_id_global = dyn_cast<GlobalVariable>(property_id_ptr);
  } else {
    property_id_global = dyn_cast<GlobalVariable>(&property_id_value);
  }

  MEMOIR_NULL_CHECK(property_id_global, "PropertyInst has NULL ID");

  auto property_id_init = property_id_global->getInitializer();
  auto property_id_constant =
      dyn_cast<llvm::ConstantDataSequential>(property_id_init);
  MEMOIR_NULL_CHECK(property_id_constant,
                    "PropertyInst ID is not a constant data array");

  return property_id_constant->getAsCString().str();
}

OPERAND(PropertyInst, PropertyIDOperand, 0);

unsigned PropertyInst::getNumberOfArguments() const {
  return this->getCallInst().getNumOperands() - 1;
}

VAR_OPERAND(PropertyInst, Argument, 1)

TO_STRING(PropertyInst)

} // namespace llvm::memoir
