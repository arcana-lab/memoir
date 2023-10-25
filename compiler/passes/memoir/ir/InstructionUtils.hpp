#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

#define RESULTANT(CLASS, NAME)                                                 \
  llvm::Value &CLASS::get##NAME() const {                                      \
    return this->getCallInst();                                                \
  }

#define OPERAND(CLASS, NAME, OP_NUM)                                           \
  llvm::Value &CLASS::get##NAME() const {                                      \
    return *(this->get##NAME##AsUse().get());                                  \
  }                                                                            \
  llvm::Use &CLASS::get##NAME##AsUse() const {                                 \
    return this->getCallInst().getOperandUse(OP_NUM);                          \
  }

#define VAR_OPERAND(CLASS, NAME, OP_OFFSET)                                    \
  llvm::Value &CLASS::get##NAME(unsigned index) const {                        \
    return *(this->get##NAME##AsUse(index).get());                             \
  }                                                                            \
  llvm::Use &CLASS::get##NAME##AsUse(unsigned index) const {                   \
    return this->getCallInst().getOperandUse(OP_OFFSET + index);               \
  }

#define TO_STRING(CLASS)                                                       \
  std::string CLASS::toString(std::string indent) const {                      \
    std::string str, llvm_str;                                                 \
    llvm::raw_string_ostream llvm_ss(llvm_str);                                \
    llvm_ss << this->getCallInst();                                            \
    str = #CLASS + llvm_str;                                                   \
    return str;                                                                \
  }
