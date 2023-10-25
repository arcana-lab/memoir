#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

// SizeInst implementation
RESULTANT(SizeInst, Size)
OPERAND(SizeInst, Collection, 0)
TO_STRING(SizeInst)

// EndInst implementation
RESULTANT(EndInst, Value)
TO_STRING(EndInst)

} // namespace llvm::memoir
