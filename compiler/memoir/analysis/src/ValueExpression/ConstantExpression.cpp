#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

llvm::Constant &ConstantExpression::getConstant() const {
  return C;
}

bool ConstantExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) {
  return true;
}

llvm::Value *ConstantExpression::materialize(llvm::Instruction &IP,
                                             MemOIRBuilder *builder,
                                             const llvm::DominatorTree *DT,
                                             llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  return &C;
}

} // namespace llvm::memoir
