#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

bool UnknownExpression::isAvailable(llvm::Instruction &IP,
                                    const llvm::DominatorTree *DT,
                                    llvm::CallBase *call_context) {
  return false;
}

llvm::Value *UnknownExpression::materialize(llvm::Instruction &IP,
                                            MemOIRBuilder *builder,
                                            const llvm::DominatorTree *DT,
                                            llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  return nullptr;
}

} // namespace llvm::memoir
