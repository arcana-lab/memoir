// MemOIR
#include "memoir/support/Assert.hpp"
#include "memoir/support/Print.hpp"

// Sequence Provisioning
#include "SequenceProvisioner.hpp"

namespace llvm::memoir {

SequenceProvisioner::~SequenceProvisioner() {
  // Do nothing.
}

// Top-level invocation.
bool SequenceProvisioner::run() {
  auto analysis_succeeded = this->analyze();
  if (analysis_succeeded) {
    return this->transform();
  }
  return false;
}

// Analysis.
bool SequenceProvisioner::analyze() {
  // Collect all SequenceAllocInsts.
  set<SequenceAllocInst *> sequence_allocations = {};
  for (auto &F : this->M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *memoir_inst = MemOIRInst::get(I)) {
          if (auto *seq_alloc_inst = dyn_cast<SequenceAllocInst>(memoir_inst)) {
            sequence_allocations.insert(seq_alloc_inst);
          }
        }
      }
    }
  }

  // Follow the sequence allocations to find those that are size-variant.
  set<SequenceAllocInst *> size_variant_sequence_allocations = {};
  map<SequenceAllocInst *, set<JoinInst *>> alloc_to_join_insts = {};
  map<SequenceAllocInst *, set<SliceInst *>> alloc_to_slice_insts = {};
  for (auto *seq_alloc_inst : sequence_allocations) {
    println("Analyzing: ", *seq_alloc_inst);

    // Get the LLVM CallInst.
    auto &llvm_inst = seq_alloc_inst->getCallInst();

    // Create a working set.
    set<llvm::Value *> workset = { &llvm_inst };
    set<llvm::Value *> visited = { &llvm_inst };
#define ADD_WORK(v)                                                            \
  if (visited.find(v) == visited.end()) {                                      \
    new_workset.insert(v);                                                     \
    visited.insert(v);                                                         \
  }

    // While the working set is non-empty, iterate over its users to see if it
    // is ever joined.
    while (!workset.empty()) {
      set<llvm::Value *> new_workset = {};
      for (auto *work_item : workset) {
        for (auto &use : work_item->uses()) {
          // Get the Use information.
          auto *used_value = use.get();
          auto *user = use.getUser();

          if (auto *user_as_inst = dyn_cast<llvm::Instruction>(user)) {
            // Check if the user is a memoir instruction.
            if (auto *memoir_inst = MemOIRInst::get(*user_as_inst)) {

              // Check if the user is a JoinInst, if so, add this
              // SequenceAllocInst to the list of size variants and record the
              // JoinInst.
              if (auto *join_inst = dyn_cast<JoinInst>(memoir_inst)) {
                size_variant_sequence_allocations.insert(seq_alloc_inst);
                alloc_to_join_insts[seq_alloc_inst].insert(join_inst);
                continue;
              }

              // Check if the user is a SliceInst, if so, record the SliceInst.
              if (auto *slice_inst = dyn_cast<SliceInst>(memoir_inst)) {
                alloc_to_slice_insts[seq_alloc_inst].insert(slice_inst);
                ADD_WORK(&slice_inst->getCallInst());
                continue;
              }

              // Check if the user is an AccessInst, if so, don't append its
              // users.
              if (isa<AccessInst>(memoir_inst)) {
                continue;
              }
            } else if (auto *call_inst = dyn_cast<llvm::CallInst>(user)) {
              // If the user is a call instruction, add the argument to the
              // workset.

              // Get the called function.
              if (auto *called_function = call_inst->getCalledFunction()) {
                // Get the argument.
                auto *argument =
                    called_function->arg_begin() + use.getOperandNo();

                // Add the argument to the workset.
                ADD_WORK(argument);
                continue;
              } else {
                // No handling for indirect calls.
                continue;
              }
            } else if (auto *return_inst = dyn_cast<llvm::ReturnInst>(user)) {
              // If the user is a return instruction, add all of the callers of
              // this function to the workset.

              // Get the function being returned from.
              auto &parent_bb = sanitize(
                  return_inst->getParent(),
                  "Return instruction does not belong to a basic block!");
              auto &parent_func =
                  sanitize(parent_bb.getParent(),
                           "Return instruction does not belong to a function!");

              // Add all CallInst users of the function to the workset.
              for (auto *func_user : parent_func.users()) {
                if (auto *call_inst = dyn_cast<llvm::CallInst>(func_user)) {
                  ADD_WORK(call_inst);
                }
              }

              continue;
            }

            // Otherwise, add the user to the workset.
            ADD_WORK(user_as_inst);
          }
        }
      }
      workset.clear();
      workset.insert(new_workset.begin(), new_workset.end());
    }
  }

  // Print the sequences that are size-variant.
  println();
  println("=============================");
  println("Size variant sequences found:");
  for (auto *size_variant_sequence : size_variant_sequence_allocations) {
    println("  ", *size_variant_sequence);
  }
  println("=============================");
  println();

  // Iterate over the size-variant sequences to determine the size to provision
  // the original array.

  // If the size variant sequence set is non-empty, analysis has succeeded.
  return false;
}

// Transformation.
bool SequenceProvisioner::transform() {
  // TODO
  return false;
}

} // namespace llvm::memoir
