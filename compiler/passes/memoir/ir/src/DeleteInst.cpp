#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InstructionUtils.hpp"

namespace llvm::memoir {

// DeleteStructInst implementation
OPERAND(DeleteStructInst, StructDeleted, 0)
TO_STRING(DeleteStructInst)

// DeleteCollectionInst implementation
OPERAND(DeleteCollectionInst, CollectionDeleted, 0)
TO_STRING(DeleteCollectionInst)

} // namespace llvm::memoir
