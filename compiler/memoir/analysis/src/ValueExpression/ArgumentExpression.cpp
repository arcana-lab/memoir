#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

bool ArgumentExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) {
  // Get the insertion basic block and function.
  auto insertion_bb = IP.getParent();
  if (!insertion_bb) {
    debugln("Couldn't get the insertion basic block");
    return false;
  }
  auto insertion_func = insertion_bb->getParent();
  if (!insertion_func) {
    debugln("Couldn't get the insertion function");
    return false;
  }

  // Check that the insertion point and the argument are in the same function.
  auto value_func = this->A.getParent();
  if (value_func == insertion_func) {
    return true;
  }

  // See if the argument is available at the calling context.
  // TODO: extend this to a have partial call stack.
  if (call_context) {
    return this->isAvailable(*call_context, DT);
  }

  // Otherwise, the argument is not available.
  return false;
}

llvm::Value *ArgumentExpression::materialize(llvm::Instruction &IP,
                                             MemOIRBuilder *builder,
                                             const llvm::DominatorTree *DT,
                                             llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  debugln("  ", this->A);
  debugln("  at ", IP);

  // Check if this Argument is available.
  if (!isAvailable(IP, DT, call_context)) {
    return nullptr;
  }

  // Get the insertion basic block and function.
  auto insertion_bb = IP.getParent();
  if (!insertion_bb) {
    debugln("Couldn't determine insertion basic block");
    return nullptr;
  }
  auto insertion_func = insertion_bb->getParent();
  if (!insertion_func) {
    debugln("Couldn't determine insertion function");
    return nullptr;
  }

  // Check that the insertion point and the argument are in the same function.
  auto value_func = this->A.getParent();
  if (value_func == insertion_func) {
    debugln("argument is available, forwarding along.");
    return &(this->A);
  }

  // See if the argument is available at the calling context.
  // TODO: extend this to have a partial call stack.
  if (call_context) {
    if (this->isAvailable(*call_context, DT)) {
      // Handle the call.
      if (auto *materialized_argument =
              handleCallContext(*this, this->A, *call_context, builder, DT)) {

        debugln("  Materialized: ", *materialized_argument);
        return materialized_argument;
      }
    }
  }

  // Otherwise, the argument is not available.
  return nullptr;
}

} // namespace llvm::memoir
