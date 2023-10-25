#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"

namespace llvm::memoir {

// DeleteStructInst implementation
OPERAND(DeleteStructInst, StructDeleted, 0)
TO_STRING(DeleteStructInst)

// DeleteCollectionInst implementation
OPERAND(DeleteCollectionInst, CollectionDeleted, 0)
TO_STRING(DeleteCollectionInst)

} // namespace llvm::memoir
