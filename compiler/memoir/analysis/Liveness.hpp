#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "noelle/core/DataFlow.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

struct LivenessResult {
public:
  /**
   * Query if a value is alive at a given MEMOIR instruction.
   *
   * @param V the value to query liveness for
   * @param I the MEMOIR instruction to query at
   * @param after query liveness immediately before (false) or after (true) the
   * instruction (default = true)
   *
   * @returns true if the value is alive, false otherwise.
   */
  bool is_live(llvm::Value &V, MemOIRInst &I, bool after = true);

  /**
   * Query if a value is alive at a given LLVM instruction.
   *
   * @param V the value to query liveness for
   * @param I the LLVM instruction to query at
   * @param after query liveness immediately before (false) or after (true) the
   * instruction (default = true)
   *
   * @returns true if the value is alive, false otherwise.
   */
  bool is_live(llvm::Value &V, llvm::Instruction &I, bool after = true);

  /**
   * Get the set of live values at a given MEMOIR instruction.
   *
   * @param I the LLVM instruction to query at
   * @param after query liveness immediately before (false) or after (true) the
   * instruction (default = true)
   *
   * @returns a reference to the set of live values
   */
  set<llvm::Value *> live_values(MemOIRInst &I, bool after = true);

  /**
   * Get the set of live values at a given LLVM instruction.
   *
   * @param I the LLVM instruction to query at
   * @param after query liveness immediately before (false) or after (true) the
   * instruction (default = true)
   *
   * @returns a reference to the set of live values
   */
  set<llvm::Value *> live_values(llvm::Instruction &I, bool after = true);

  /**
   * Get the set of live values along a control edge between two basic blocks.
   *
   * @param From the LLVM basic block that is the source of the edge
   * @param To the LLVM basic block that is the desination of the edge
   *
   * @returns the set of live values
   */
  set<llvm::Value *> live_values(llvm::BasicBlock &From, llvm::BasicBlock &To);

  /**
   * Get the underlying NOELLE data flow result.
   *
   * @returns a reference to the data flow result
   */
  arcana::noelle::DataFlowResult &get_dataflow_result();

protected:
  arcana::noelle::DataFlowResult *DFR;
};

class LivenessDriver {
public:
  LivenessDriver(llvm::Function &F, arcana::noelle::DataFlowEngine DFE);

protected:
  LivenessResult &result;

  llvm::Function &F;
  arcana::noelle::DataFlowEngine DFE;
};

LivenessResult LiveAnalysis::run(llvm::Function &F,
                                 llvm::FunctionAnalysisManager &FAM) {
  // Construct a new result.
  LivenessResult result;

  // Get the ModuleAnalysisManager proxy.
  auto &MAM = GET_MODULE_ANALYSIS_MANAGER(FAM, F);

  // Get NOELLE from the analysis manager.
  auto &NOELLE = MAM.getResult<arcana::noelle::Noelle>(M);

  // Get the DataFlowEngine from NOELLE.
  auto &DFE = NOELLE.getDataFlowEngine();

  // Run the analysis.
  LivenessDriver driver(F, DFE);

  // Return the analysis.
  return result;
}

} // namespace llvm::memoir
