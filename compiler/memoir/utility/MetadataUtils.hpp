#include "memoir/utility/Metadata.hpp"

#define OPERAND(CLASS, NAME, OP_NUM)                                           \
  llvm::Metadata &CLASS::get##NAME##MD() const {                               \
    return *(this->get##NAME##MDOperand().get());                              \
  }                                                                            \
  const llvm::MDOperand &CLASS::get##NAME##MDOperand() const {                 \
    return this->getMetadata().getOperand(OP_NUM);                             \
  }

#define VALUE_OPERAND(CLASS, NAME, OP_NUM)                                     \
  llvm::Value &CLASS::get##NAME() const {                                      \
    auto &metadata = this->getArgumentMD();                                    \
    auto &value_as_metadata =                                                  \
        MEMOIR_SANITIZE( \ 
      dyn_cast<llvm::ValueAsMetadata>(&argument_metadata),                     \
                         #NAME " operand of " #CLASS                           \
                               " is not a ValueAsMetadata");                   \
    auto &value =                                                              \
        MEMOIR_SANITIZE(argument_value_as_metadata.getValue(),                 \
                        #NAME " operand of " #CLASS " is a NULL value!");      \
    return value;                                                              \
  }                                                                            \
  llvm::MDNode &CLASS::get##NAME##MD() const {                                 \
    return *(this->get##NAME##MDOperand().get());                              \
  }                                                                            \
  const llvm::MDOperand &CLASS::get##NAME##MDOperand() const {                 \
    return this->getMetadata().getOperand(OP_NUM);                             \
  }
