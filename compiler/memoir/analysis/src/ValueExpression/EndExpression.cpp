#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

bool EndExpression::isAvailable(llvm::Instruction &IP,
                                const llvm::DominatorTree *DT,
                                llvm::CallBase *call_context) {
  debugln("Checking availability ", *this);

  // TODO!
  return false;
}

llvm::Value *EndExpression::materialize(llvm::Instruction &IP,
                                        MemOIRBuilder *builder,
                                        const llvm::DominatorTree *DT,
                                        llvm::CallBase *call_context) {
  debugln("Materializing ", *this);

  // TODO!
  return nullptr;
}

} // namespace llvm::memoir
