#include "memoir/ir/Instructions.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// DeleteInst implementation
OPERAND(DeleteInst, Object, 0)
TO_STRING(DeleteInst)

} // namespace llvm::memoir
