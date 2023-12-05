#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "noelle/core/DataFlow.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {
class LivenessAnalysis {
public:
  // Construction and analysis invocation.
  LivenessAnalysis(llvm::Function &F, arcana::noelle::DataFlowEngine DFE);

  // Queries.
  bool is_live(llvm::Value &V, MemOIRInst &I, bool after = true);
  bool is_live(llvm::Value &V, llvm::Instruction &I, bool after = true);
  std::set<llvm::Value *> &get_live_values(MemOIRInst &I, bool after = true);
  std::set<llvm::Value *> &get_live_values(llvm::Instruction &I,
                                           bool after = true);

protected:
  llvm::Function &F;
  arcana::noelle::DataFlowEngine DFE;
  arcana::noelle::DataFlowResult *DFR;
};

} // namespace llvm::memoir
