#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// AssocKeysInst implementation.
RESULTANT(AssocKeysInst, Keys)
OPERAND(AssocKeysInst, Collection, 0)
TO_STRING(AssocKeysInst)

} // namespace llvm::memoir
