#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InstructionUtils.hpp"

namespace llvm::memoir {

// DeleteStructInst implementation
OPERAND(DeleteStructInst, DeletedStruct, 0)
TO_STRING(DeleteStructInst)

// DeleteCollectionInst implementation
OPERAND(DeleteCollectionInst, DeletedCollection, 0)
TO_STRING(DeleteCollectionInst)

} // namespace llvm::memoir
