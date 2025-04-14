#include <iostream>
#include <string>

// LLVM
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

// MemOIR
#include "memoir/ir/Instructions.hpp"
#include "memoir/passes/Passes.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/support/UnionFind.hpp"
#include "memoir/utility/Metadata.hpp"

using namespace llvm::memoir;

/*
 * This pass erases all existing LiveOutMetadata and re-inserts it.
 *
 * Author(s): Tommy McMichen
 * Created: September 24, 2024
 */

namespace llvm::memoir {

namespace detail {

void _gather_reaching_definitions(llvm::Value &V,
                                  Set<llvm::Value *> &reaching) {

  if (reaching.count(&V) > 0) {
    return;
  } else {
    reaching.insert(&V);
  }

  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    if (auto *memoir_inst = into<MemOIRInst>(user)) {

      if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {

        // If the use is the updated object, propagate it.
        if (use == update->getObjectAsUse()) {
          _gather_reaching_definitions(*user, reaching);
        }

      } else if (isa<RetPHIInst>(memoir_inst) or isa<UsePHIInst>(memoir_inst)) {

        // Merge the user with the reaching definitions of the value.
        _gather_reaching_definitions(*user, reaching);

      } else if (auto *fold = into<FoldInst>(user)) {

        // If the value is being accumulated, merge it.
        if (&use == &fold->getInitialAsUse()) {
          _gather_reaching_definitions(*user, reaching);
        }
      }
    } else if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      _gather_reaching_definitions(*user, reaching);
    }
  }

  return;
}

Set<llvm::Value *> gather_reaching_definitions(llvm::Value &V) {

  Set<llvm::Value *> reaching;

  _gather_reaching_definitions(V, reaching);

  return reaching;
}

llvm::Value *find_single_postdominator(llvm::PostDominatorTree &PDT,
                                       llvm::ArrayRef<llvm::Value *> values) {

  // If we only have a single value, return it.
  if (values.size() == 1) {
    return values.front();
  }

  debugln();

  // Now find the value that post dominates all other values.
  for (auto *val : values) {

    debugln("VAL ", *val);

    auto *inst = dyn_cast<llvm::Instruction>(val);
    if (not inst) {
      continue;
    }

    bool postdom_all = true;
    for (auto *other : values) {
      // Skip itself.
      if (val == other) {
        continue;
      }

      auto *other_inst = dyn_cast<llvm::Instruction>(other);
      if (not other_inst) {
        continue;
      }

      // Check postdominance.
      if (not PDT.dominates(inst, other_inst)) {
        debugln("  FAILED ", *other_inst);
        postdom_all = false;
        break;
      }
    }

    if (postdom_all) {
      debugln("  POSTDOM ALL");
      return val;
    }
  }

  return nullptr;
}

} // namespace detail

llvm::PreservedAnalyses LiveOutInsertionPass::run(
    llvm::Function &F,
    llvm::FunctionAnalysisManager &FAM) {

  infoln();
  infoln("BEGIN Live-Out Insertion pass");
  infoln();

  // Remove all LiveOutMetadata from the function.
  for (auto &BB : F) {
    for (auto &I : BB) {
      Metadata::remove<LiveOutMetadata>(I);
    }
  }

  // Collect all of the collection-typed arguments.
  Set<llvm::Argument *> arguments = {};
  for (auto &A : F.args()) {
    if (auto *type = type_of(A)) {
      if (isa<SequenceType>(type) or isa<AssocArrayType>(type)) {
        arguments.insert(&A);
      }
    }
  }

  // Find the single return instruction.
  llvm::ReturnInst *single_return = nullptr;
  for (auto &BB : F) {
    // Fetch the basic block's terminator.
    auto *terminator = BB.getTerminator();

    // Ensure that the terminator is a return.
    auto *ret = dyn_cast<llvm::ReturnInst>(terminator);
    if (not ret) {
      continue;
    }

    if (single_return) {
      MEMOIR_UNREACHABLE("Function has mutliple returns!");
    }

    single_return = ret;
  }

  // Fetch the dominator tree.
  auto &DT = FAM.getResult<llvm::DominatorTreeAnalysis>(F);

  // Fetch the post-dominator tree.
  auto &PDT = FAM.getResult<llvm::PostDominatorTreeAnalysis>(F);

  // For each argument, find the reaching definition at the single return.
  for (auto *arg : arguments) {

    // Gather the reaching definitions.
    auto reaching = detail::gather_reaching_definitions(*arg);

    // If the argument is the only value in the set of reaching definitions,
    // return early.
    if (reaching.size() == 1) {
      continue;
    }

    // Filter the reaching definitions to get the subset that dominate the
    // return.
    Vector<llvm::Value *> dominates = {};
    std::copy_if(
        reaching.begin(),
        reaching.end(),
        std::back_inserter(dominates),
        [&](llvm::Value *val) { return DT.dominates(val, single_return); });

    // Now find the value that post dominates all other values.
    auto *postdominates = detail::find_single_postdominator(PDT, dominates);

    // If we failed to find a single instruction, we will do a small hack to
    // handle functions that call exit().
    if (not postdominates) {
      // Find all values that are in the same basic block as the single return.
      Vector<llvm::Value *> local = {};
      std::copy_if(dominates.begin(),
                   dominates.end(),
                   std::back_inserter(local),
                   [&](llvm::Value *val) {
                     auto *inst = dyn_cast<llvm::Instruction>(val);
                     if (not inst) {
                       return false;
                     }
                     return inst->getParent() == single_return->getParent();
                   });

      postdominates = detail::find_single_postdominator(PDT, local);

      if (not postdominates) {
        println(*arg->getParent());
        MEMOIR_UNREACHABLE(
            "Failed to find a value that dominates function exit "
            "and postdominates all others for argument ",
            *arg,
            " in ",
            arg->getParent()->getName());
      }
    }

    // Insert the live out metadata.
    if (auto *postdom_inst = dyn_cast<llvm::Instruction>(postdominates)) {
      auto live_out_metadata =
          Metadata::get_or_add<LiveOutMetadata>(*postdom_inst);
      live_out_metadata.setArgNo(arg->getArgNo());
    }
  }

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
