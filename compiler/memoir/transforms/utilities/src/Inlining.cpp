// LLVM
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// MEMOIR
#include "memoir/transforms/utilities/Inlining.hpp"

#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

/**
 * Helper function to collect the set of basic blocks between the start and end
 * block. This function assumes that start and end are the entry and exit of an
 * SESE region.
 *
 * @param blocks the set of basic blocks that will be populated with the basic
 *               blocks in the region, not including entry and exit
 * @param entry a reference to the single-entry
 * @param exit a reference to the single-exit
 */
void collect_blocks_between(set<llvm::BasicBlock *> &blocks,
                            llvm::BasicBlock &entry,
                            llvm::BasicBlock &exit) {
  vector<llvm::BasicBlock *> worklist = { &entry };
  while (not worklist.empty()) {
    // Pop off the top block of the worklist.
    auto *current = worklist.back();
    worklist.pop_back();

    // For each successor, add it to the set of blocks:
    for (auto *successor : llvm::successors(current)) {
      // Sanity check that the basic block is non-NULL.
      if (successor == nullptr) {
        continue;
      }

      // If the block is the entry or exit, skip it.
      if (successor == &entry or successor == &exit) {
        continue;
      }

      // If the block is already in the set of blocks, skip it.
      if (blocks.count(successor) > 0) {
        continue;
      }

      // Otherwise, add it to the set and the worklist.
      blocks.insert(successor);
      worklist.push_back(successor);
    }
  }
  return;
}

llvm::InlineResult InlineFunction(llvm::CallBase &CB,
                                  llvm::InlineFunctionInfo &IFI,
                                  bool merge_attributes,
                                  llvm::AAResults *callee_aa_results,
                                  bool insert_lifetime,
                                  llvm::Function *forward_args_to) {

  // Split the call into its own basic block.
  auto *block = CB.getParent();
  auto *entry =
      llvm::SplitBlock(block,
                       &CB,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "inline.entry.",
                       /* Before = */ true);

  auto *exit =
      llvm::SplitBlock(block,
                       CB.getNextNode(),
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "inline.exit.",
                       /* Before = */ false);

  // Save the function operand for later.
  auto *callee = CB.getCalledOperand();

  // Collect all of the collection arguments to the function before we inline
  // the function.
  unsigned arg_index = 0;
  map<unsigned, llvm::Value *> arguments_to_patch = {};
  for (auto &arg_use : CB.data_ops()) {

    // Unpack the argument value.
    auto *arg = arg_use.get();

    // If the argument is a collection, add it to the mapping.
    if (Type::value_is_collection_type(*arg)) {
      arguments_to_patch[arg_index] = arg;
    }

    // Increment the argument index.
    ++arg_index;
  }

  // Inline the function at the call site.
  auto result = llvm::InlineFunction(CB,
                                     IFI,
                                     merge_attributes,
                                     callee_aa_results,
                                     insert_lifetime,
                                     forward_args_to);

  // If we did not a succeed, return now.
  if (not result.isSuccess()) {
    return result;
  }

  // If there are no arguments to patch, return now.
  if (arguments_to_patch.empty()) {
    return result;
  }

  // Collect the set of basic blocks introduced by inlining.
  set<llvm::BasicBlock *> inlined_blocks = {};
  collect_blocks_between(inlined_blocks, *entry, *exit);

  // Otherwise, we need to traverse the CFG of the newly inlined function to
  // find any instructions with LiveOutMetadata attached.
  map<llvm::Value *, llvm::Value *> patches = {};
  for (auto *inlined_block : inlined_blocks) {
    for (auto &I : *inlined_block) {
      // Check if this instruction has live-out metadata.
      auto live_out_metadata = Metadata::get<LiveOutMetadata>(I);

      // If it doesn't, skip it.
      if (not live_out_metadata.has_value()) {
        continue;
      }

      // Otherwise, get the corresponding argument.
      auto arg_number = live_out_metadata->getArgNo();
      if (arguments_to_patch.count(arg_number) == 0) {
        MEMOIR_UNREACHABLE(
            "LiveOutMetadata has invalid/stale argument number.");
      }
      auto *argument = arguments_to_patch[arg_number];

      // Record the patch.
      if (patches.count(argument) > 0) {
        MEMOIR_UNREACHABLE(
            "Argument of inlined function has multiple live-outs."
            "The function had more than one ReturnInst!");
      }
      patches[argument] = &I;

      // Remove the old metadata.
      Metadata::remove<LiveOutMetadata>(I);
    }
  }

  // Find all relevant RetPHIs.
  set<llvm::Instruction *> to_cleanup = {};
  for (const auto &[to_patch, patch] : patches) {

    // For each use of the value to patch:
    for (auto &use : to_patch->uses()) {
      // Get the user information.
      auto *user = use.getUser();
      auto *user_as_inst = dyn_cast<llvm::Instruction>(user);

      // Skip non-instruction users.
      if (not user_as_inst) {
        continue;
      }

      // FIXME: this implementation assumes that there are no other calls to the
      // same function, it needs to be extended with dominance information
      // (checking that the RetPHI is dominated by the patch).

      // If the instruction is a RetPHI, add it to the set if it is for the same
      // callee.
      if (auto *ret_phi = into<RetPHIInst>(*user_as_inst)) {

        // Check that the callee is the same.
        if (&ret_phi->getCalledOperand() != callee) {
          continue;
        }

        // Get the input collection.
        auto &input = ret_phi->getInputCollection();

        // Replace uses of the RetPHI with the patch.
        ret_phi->getResultCollection().replaceAllUsesWith(patch);

        // Mark the RetPHI for cleanup.
        to_cleanup.insert(&ret_phi->getCallInst());
      }
    }
  }

  // Erase the old RetPHIs.
  for (auto *cleanup : to_cleanup) {
    cleanup->eraseFromParent();
  }

  return result;
}

} // namespace llvm::memoir
