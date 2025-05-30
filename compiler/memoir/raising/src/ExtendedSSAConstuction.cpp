#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/passes/Passes.hpp"
#include "memoir/raising/ExtendedSSAConstruction.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/DominatorUtils.hpp"

namespace llvm::memoir {

bool construct_extended_ssa(llvm::Function &F, llvm::DominatorTree &DT) {

  // First, split all critical edges in the function.
  llvm::CriticalEdgeSplittingOptions options(&DT);
  auto split_edges = llvm::SplitAllCriticalEdges(F, options);

  // Find all of the conditional branches that may need pi-nodes inserted.
  Vector<llvm::BranchInst *> checks = {};
  for (auto &BB : F) {
    auto *terminator = BB.getTerminator();

    // Skip non-branches.
    auto *branch = dyn_cast<llvm::BranchInst>(terminator);
    if (not branch) {
      continue;
    }

    // Skip unconditional branches.
    if (not branch->isConditional()) {
      continue;
    }

    // Gather the values that need to be split.
    checks.push_back(branch);
  }

  // If there are no checks to handle, then return.
  if (checks.empty()) {
    return split_edges > 0;
  }

  // Order the branches in level-order.
  std::sort(checks.begin(),
            checks.end(),
            [&DT](llvm::BranchInst *lhs, llvm::BranchInst *rhs) {
              // Get the parent basic blocks.
              auto *lhs_block = lhs->getParent();
              auto *rhs_block = rhs->getParent();

              // Partial domtree level order between basic block.
              auto *lhs_node = DT[lhs_block];
              auto *rhs_node = DT[rhs_block];

              MEMOIR_ASSERT(lhs_node, "LHS NODE = NULL");
              MEMOIR_ASSERT(rhs_node, "RHS NODE = NULL");

              auto lhs_level = lhs_node->getLevel();
              auto rhs_level = rhs_node->getLevel();

              if (lhs_level < rhs_level) {
                return true;
              } else if (rhs_level < lhs_level) {
                return false;
              }

              if (lhs_block == rhs_block) {
                return lhs->comesBefore(rhs);
              }

              return lhs_block < rhs_block;
            });

  // Create a builder for the function.
  auto *entry_point = F.getEntryBlock().getFirstNonPHI();
  MemOIRBuilder builder(entry_point);

  // Insert PHIs to split the live range of each value.
  Map<llvm::Value *, Vector<llvm::PHINode *>> splitting_phis = {};
  for (auto *branch : checks) {
    // Get the if-else blocks.
    auto *then_block = branch->getSuccessor(0);
    auto *else_block = branch->getSuccessor(1);

    // Fetch the condition.
    auto *cond = branch->getCondition();

    // Fetch the values whose live ranges we need to split.
    Vector<llvm::Value *> to_split = {};
    if (auto *has = into<HasInst>(cond)) {
      auto &object = has->getObject();
      to_split.push_back(&object);
    } else if (auto *cmp = dyn_cast<llvm::CmpInst>(cond)) {
      // TODO: handle arbitrary comparisons.
    }

    // Insert a PHI at the beginning of the then block.
    auto split_live_range = [&builder, branch](
                                llvm::Value &value,
                                llvm::BasicBlock &block) -> llvm::PHINode * {
      // Set the insert point to the beginning of the basic block.
      builder.SetInsertPoint(block.begin());

      // We assume that critical edge splitting has already been run, so we
      // will only have _one_ incoming edge.
      auto *phi = builder.CreatePHI(value.getType(), 1, "pi.");

      // Add the incoming value from the single predecessor.
      phi->addIncoming(&value, branch->getParent());

      // Patch up all dominated uses.
      // value.replaceUsesWithIf(&value, [&](const llvm::Use &use) -> bool {
      // return DT.dominates(phi, use);
      // });

      return phi;
    };

    // Split the live range of each value.
    for (auto *val : to_split) {
      // Split the live range in the then block.
      auto *then_phi = split_live_range(*val, *then_block);
      splitting_phis[val].push_back(then_phi);

      // Split the live range in the else block.
      auto *else_phi = split_live_range(*val, *else_block);
      splitting_phis[val].push_back(else_phi);
    }

    // Use the dominance frontier to insert PHI nodes at control-flow joins.
    Vector<llvm::PHINode *> join_phis = {};
  }

  // Introduce a stack variable for each value whose live range was split.
  Vector<llvm::AllocaInst *> slots = {};
  for (const auto &[val, phis] : splitting_phis) {

    // Fetch the type of the value.
    auto *type = val->getType();

    // Create a new stack allocation for the value.
    builder.SetInsertPoint(entry_point);
    auto *ptr = builder.CreateAlloca(type);
    slots.push_back(ptr);

    // After the value definition, store the value.
    auto *point = dyn_cast<llvm::Instruction>(val);
    if (not point) {
      point = ptr;
    }
    builder.SetInsertPoint(point->getNextNode());
    builder.CreateStore(val, ptr);

    // At each phi, store the value.
    for (auto *phi : phis) {
      builder.SetInsertPoint(phi->getParent()->getFirstInsertionPt());
      builder.CreateStore(phi, ptr);
    }

    // At each use of the value, replace it with a load from the stack slot.
    for (auto &use : val->uses()) {
      auto *user = dyn_cast<llvm::Instruction>(use.getUser());

      // Skip non-instruction uses.
      if (not user) {
        continue;
      }

      // Insert a load before the user.
      builder.SetInsertPoint(user);
      auto *load = builder.CreateLoad(type, ptr);

      // Replace the value with the loaded value.
      use.set(load);
    }
  }

  // Check that all slots can be promotable.
  Vector<llvm::AllocaInst *> to_promote = {};
  for (auto *slot : slots) {
    if (llvm::isAllocaPromotable(slot)) {
      to_promote.push_back(slot);

    } else {
      warnln("Cannot promote alloca introduced by extended SSA ", *slot);
    }
  }

  // Promote all of the newly introduced stack slots to registers.
  llvm::PromoteMemToReg(to_promote, DT);

  return true;
}

llvm::PreservedAnalyses ExtendedSSAConstructionPass::run(
    llvm::Function &F,
    llvm::FunctionAnalysisManager &FAM) {

  // Fetch the dominator tree.
  auto &DT = FAM.getResult<llvm::DominatorTreeAnalysis>(F);

  // Transform the function.
  auto modified = construct_extended_ssa(F, DT);

  // Report which analyses are preserved.
  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

} // namespace llvm::memoir
