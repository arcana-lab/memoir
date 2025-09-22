#include "llvm/IR/Constants.h"

#include "memoir/ir/MutOperations.hpp"

#include "memoir/ir/Types.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace memoir {

// MutWriteInst implementation
OPERAND(MutWriteInst, ValueWritten, 0)
OPERAND(MutWriteInst, Object, 1)
TO_STRING(MutWriteInst, "mut.write")

// MutInsertInst implementation
OPERAND(MutInsertInst, Object, 0)
TO_STRING(MutInsertInst, "mut.insert")

// MutRemoveInst implementation.
OPERAND(MutRemoveInst, Object, 0)
TO_STRING(MutRemoveInst, "mut.remove")

// MutClearInst implementation
OPERAND(MutClearInst, Object, 0)
TO_STRING(MutClearInst, "mut.clear")

} // namespace memoir
