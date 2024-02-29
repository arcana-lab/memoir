#include "memoir/ir/Instructions.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// DeleteStructInst implementation
OPERAND(DeleteStructInst, DeletedStruct, 0)
TO_STRING(DeleteStructInst)

// DeleteCollectionInst implementation
OPERAND(DeleteCollectionInst, DeletedCollection, 0)
TO_STRING(DeleteCollectionInst)

} // namespace llvm::memoir
