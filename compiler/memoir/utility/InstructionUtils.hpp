#include "memoir/ir/Instructions.hpp"

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

#define TO_STRING(CLASS, OP)                                                   \
  std::string CLASS::toString() const {                                        \
    std::string str, llvm_str;                                                 \
    llvm::raw_string_ostream llvm_ss(llvm_str);                                \
    this->asValue().printAsOperand(llvm_ss, /*type?*/ false);                  \
    str = llvm_str                                                             \
          + " = "                                                              \
            "memoir." OP "(";                                                  \
    for (auto &arg : this->getCallInst().args()) {                             \
      llvm_ss.flush();                                                         \
      arg.get()->printAsOperand(llvm_ss);                                      \
      str += llvm_str;                                                         \
    }                                                                          \
    str += ")";                                                                \
    return str;                                                                \
  }
