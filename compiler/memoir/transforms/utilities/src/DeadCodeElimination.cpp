#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/Local.h"

#include "memoir/passes/Passes.hpp"
#include "memoir/support/WorkList.hpp"
#include "memoir/transforms/utilities/DeadCodeElimination.hpp"
#include "memoir/utility/Metadata.hpp"

namespace memoir {

static bool is_trivially_dead(llvm::Instruction &inst,
                              const llvm::TargetLibraryInfo *TLI = NULL) {
  return llvm::isInstructionTriviallyDead(&inst, TLI)
         and not Metadata::get<LiveOutMetadata>(inst);
}

static bool eliminate_dead_code(llvm::Instruction &inst,
                                WorkList<llvm::Instruction *> worklist,
                                const llvm::TargetLibraryInfo *TLI = NULL) {
  if (is_trivially_dead(inst, TLI)) {

    // Null out all of the instruction's operands to see if any operand becomes
    // dead as we go.
    for (unsigned i = 0, e = inst.getNumOperands(); i != e; ++i) {
      auto *operand = inst.getOperand(i);
      inst.setOperand(i, NULL);

      if (not operand->use_empty() or &inst == operand)
        continue;

      // If the operand is an instruction that became dead as we nulled out the
      // operand, and if it is 'trivially' dead, delete it in a future loop
      // iteration.
      if (auto *op_inst = dyn_cast<llvm::Instruction>(operand))
        if (is_trivially_dead(*op_inst, TLI))
          worklist.push(op_inst);
    }

    inst.eraseFromParent();
    return true;
  }
  return false;
}

bool eliminate_dead_code(llvm::Function &function,
                         llvm::TargetLibraryInfo *TLI) {
  bool changed = false;
  WorkList<llvm::Instruction *> worklist;

  // Iterate over the original function, only adding insts to the worklist
  // if they actually need to be revisited. This avoids having to pre-init
  // the worklist with the entire function's worth of instructions.
  for (auto &inst : llvm::make_early_inc_range(llvm::instructions(function))) {
    // We're visiting this instruction now, so make sure it's not in the
    // worklist from an earlier visit.
    if (not worklist.present(&inst))
      changed |= eliminate_dead_code(inst, worklist, TLI);
  }

  while (!worklist.empty()) {
    auto *inst = worklist.pop();
    changed |= eliminate_dead_code(*inst, worklist, TLI);
  }

  return changed;
}

llvm::PreservedAnalyses DeadCodeEliminationPass::run(
    llvm::Function &F,
    llvm::FunctionAnalysisManager &FAM) {
  auto modified =
      eliminate_dead_code(F, &FAM.getResult<llvm::TargetLibraryAnalysis>(F));

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

} // namespace memoir
