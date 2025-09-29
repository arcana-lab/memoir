// LLVM
#include "llvm/IR/Verifier.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Value.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/support/UnionFind.hpp"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Verifier.hpp"

// #include "memoir/analysis/Liveness.hpp"

namespace memoir {

// Temporary Declaration for testing.
struct LivenessResult {
  bool is_live(llvm::Value &V, MemOIRInst &I, bool after = true);
  bool is_live(llvm::Value &V, llvm::Instruction &I, bool after = true);
  Set<llvm::Value *> live_values(MemOIRInst &I, bool after = true);
  Set<llvm::Value *> live_values(llvm::Instruction &I, bool after = true);
  Set<llvm::Value *> live_values(llvm::BasicBlock &From, llvm::BasicBlock &To);
};

void gather_variables(UnionFind<llvm::Value *> &reaching_definition,
                      llvm::Value &V) {
  // If V is a PHI node, merge it with its incoming values.
  if (auto *phi = dyn_cast<llvm::PHINode>(&V)) {
    for (auto &incoming : phi->incoming_values()) {
      auto *incoming_value = incoming.get();
      reaching_definition.merge(phi, incoming_value);
    }
  }

  return;
}

bool check_live_set(
    const OrderedMultiMap<llvm::Value *, llvm::Value *> &partition,
    const Set<llvm::Value *> &live_set) {

  // If there is no more than one live variable, we don't need to do any further
  // checks.
  if (live_set.size() <= 1) {
    return true;
  }

  // For each partition, check that _at most one_ variable is live.
  for (auto it = partition.cbegin(); it != partition.cend();) {

    auto *partition_key = it->first;

    llvm::Value *found_one = nullptr;
    for (; it != partition.upper_bound(partition_key); ++it) {

      // If this variable is live:
      if (live_set.count(it->second) != 0) {
        // If we have already found a live variable in this partition, fail!
        if (found_one != nullptr) {
          println("Two reaching definitions alive!");
          println("  ", *found_one);
          println("  ", *(it->second));
          return false;
        }

        // Otherwise, report that we found one and continue.
        found_one = it->second;
      }
    }
  }

  // If we have gotten this far, we have succeeded!
  return true;
}

bool check_basic_block_edge(
    LivenessResult &LR,
    const OrderedMultiMap<llvm::Value *, llvm::Value *> &partition,
    llvm::BasicBlock &from,
    llvm::BasicBlock &to) {

  // Get the set of live values along this basic block edge.
  const auto &live_set = LR.live_values(from, to);

  // If we have gotten this far, we have succeeded!
  return check_live_set(partition, live_set);
}

bool check_instruction(
    LivenessResult &LR,
    const OrderedMultiMap<llvm::Value *, llvm::Value *> &partition,
    llvm::Instruction &I) {

  // Get the set of live values before this instruction.
  const auto &live_set = LR.live_values(I, /* after = */ false);

  // Check the live set against the partition.
  return check_live_set(partition, live_set);
}

bool check_call(const OrderedMultiMap<llvm::Value *, llvm::Value *> &partition,
                llvm::CallBase &call) {
  // Check that, no two collection operands are the same.

  // We will store the set of collection arguments we have already found in
  // collection_arguments.
  Set<llvm::Value *> collection_arguments = {};

  // For each argument of the call.
  for (auto &argument : call.data_ops()) {

    // Get the argument value.
    auto *argument_value = argument.get();
    if (argument_value == nullptr) {
      continue;
    }

    // If the value is a collection type, make sure it's not in the set of
    // collection arguments already.
    if (isa_and_nonnull<CollectionType>(type_of(*argument_value))) {

      // If it is, the call instruction is broken.
      if (collection_arguments.count(argument_value) > 0) {
        println("Collection duplicated!");
        println("  ", *argument_value);
        println("  at call site ", call);
        return false;
      }
    }
  }

  return true;
}

bool verify_linearity(llvm::Function &F, LivenessResult &LR) {
  // Only verify functions with contents.
  if (F.empty()) {
    return true;
  }

  // For a variable to be linearly typed, there may be at most one reaching
  // definition of it alive at each program point.

  // To do this, we will start by creating a mapping from each variable to its
  // parent definition.
  UnionFind<llvm::Value *> reaching_definition = {};

  // First, find any variable defined by a MEMOIR instruction.
  // Merge them with their MEMOIR collection operand.
  for (auto &BB : F) {
    for (auto &I : BB) {

      // Determine if this instruction is a MEMOIR instruction.
      auto *memoir_inst = into<MemOIRInst>(I);
      if (not memoir_inst) {
        // Skip non-MEMOIR instructions.
        continue;
      }

      // If this is a collection operation that returns a new collection, add it
      // to the set of variables.
      if (isa<AllocInst>(memoir_inst) or isa<KeysInst>(memoir_inst)
          or isa<CopyInst>(memoir_inst)) {
        reaching_definition.insert(&I);
      }

      // Merge the reaching definitions for the input and redefinition.
      else if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {
        auto &operand = update->getObject();
        reaching_definition.merge(&update->getResult(), &operand);
        gather_variables(reaching_definition, operand);
      }

      // If this is an AssertCollectionType instruction, add the collection to
      // the set of variables.
      else if (auto *assert_inst = dyn_cast<AssertTypeInst>(memoir_inst)) {
        reaching_definition.insert(&assert_inst->getObject());
      }
    }
  }

  // If there were no reaching definitions found, return.
  if (reaching_definition.size() == 0) {
    return true;
  }

  // Then, gather all of the redefinitions of these variables by PHI nodes.
  debugln("Reaching definitions:");
  for (const auto &[var, def] : reaching_definition) {

    debugln("  ", *var, " derived from ", *def);

    for (auto &use : var->uses()) {
      // Get the user.
      auto *user = use.getUser();
      if (not user) {
        continue;
      }

      // If the user is a PHI node, add it to the set of variables.
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        reaching_definition.merge(phi, def);
      }
    }
  }

  // Now, partition the UnionFind by inverting the mapping from variable to
  // parent definition.
  OrderedMultiMap<llvm::Value *, llvm::Value *> partition = {};
  for (const auto &[var, def] : reaching_definition) {
    // If var == def, add it to the roots.
    partition.insert({ def, var });
  }

  // At each program point, check that _at most one_ variable within each
  // partition is alive.
  for (auto &BB : F) {

    // Check each edge from this basic block to its children.
    for (auto *succ : llvm::successors(&BB)) {
      auto &Succ = MEMOIR_SANITIZE(succ, "Successor of BasicBlock is NULL!");
      if (not check_basic_block_edge(LR, partition, BB, Succ)) {
        println("  at edge from ");
        BB.printAsOperand(llvm::errs(), false);
        println(" to ");
        Succ.printAsOperand(llvm::errs(), false);
        return false;
      }
    }

    for (auto &I : BB) {
      // Skip PHIs, we will each basic block edge individually.
      if (isa<llvm::PHINode>(&I)) {
        continue;
      }

      // Check each instruction.
      if (not check_instruction(LR, partition, I)) {
        println("  at instruction ", I);
        return false;
      }

      // If the instruction is a call, check that it doesn't duplicate the
      // collection.
      if (auto *call = dyn_cast<llvm::CallBase>(&I)) {
        if (not check_call(partition, *call)) {
          return false;
        }
      }
    }
  }

  return true;
}

// Top-level queries.
bool Verifier::verify(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {
  // First, have LLVM verify that this is a valid LLVM function.
  if (llvm::verifyFunction(F, &llvm::errs())) {
    println("LLVM Verifier failed on ", F.getName());
    return true;
  }

  // TEMPORARY
  return false;

  // Get the liveness analysis result for this function.
  auto &LR = FAM.getResult<LivenessAnalysis>(F);

  // Verify that each MEMOIR collection is linearly typed.
  if (not verify_linearity(F, LR)) {
    return true;
  }

  return false;
}

bool Verifier::verify(llvm::Module &M, llvm::ModuleAnalysisManager &MAM) {
  // First, have LLVM verify that this is a valid LLVM module.
  if (llvm::verifyModule(M, &llvm::errs())) {
    println("LLVM Verifier failed");
    return true;
  }

  // Verify each function in the module.
  for (auto &F : M) {
    // Get the analysis manager for this function.
    auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);

    // Verify the function.
    if (Verifier::verify(F, FAM)) {
      println("MEMOIR Verifier failed on ", F.getName());
      return true;
    }
  }

  return false;
}

} // namespace memoir
