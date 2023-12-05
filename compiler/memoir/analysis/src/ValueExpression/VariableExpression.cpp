#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

bool VariableExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) {
  return false;
}

llvm::Value *VariableExpression::materialize(llvm::Instruction &IP,
                                             MemOIRBuilder *builder,
                                             const llvm::DominatorTree *DT,
                                             llvm::CallBase *call_context) {
  println("Materializing ", *this);
  return nullptr;
}

} // namespace llvm::memoir
