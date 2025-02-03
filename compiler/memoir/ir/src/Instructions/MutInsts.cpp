#include "llvm/IR/Constants.h"

#include "memoir/ir/MutOperations.hpp"

#include "memoir/ir/Types.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// MutWriteInst implementation
OPERAND(MutWriteInst, ValueWritten, 0)
OPERAND(MutWriteInst, Object, 1)
TO_STRING(MutWriteInst)

// MutInsertInst implementation
OPERAND(MutInsertInst, Object, 0)
TO_STRING(MutInsertInst)

// MutRemoveInst implementation.
OPERAND(MutRemoveInst, Object, 0)
TO_STRING(MutRemoveInst)

// MutClearInst implementation
OPERAND(MutClearInst, Object, 0)
TO_STRING(MutClearInst)

} // namespace llvm::memoir
