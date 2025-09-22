#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/passes/Passes.hpp"
#include "memoir/raising/Defunctionalize.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/DataTypes.hpp"

namespace llvm::memoir {

static bool address_taken(llvm::Value &val) {

  for (auto &use : val.uses()) {
    auto *user = use.getUser();

    // If the user is a call, and this value is the function operand, its
    // address is not taken.
    if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      if (call->isCallee(&use)) {
        continue;
      }
    }

    // Otherwise, the address is taken!
    return true;
  }

  return false;
}

bool defunctionalize(llvm::CallBase &call,
                     bool possibly_unknown,
                     llvm::ArrayRef<llvm::Function *> possible_callees) {

  // Unpack the call.
  auto *callee = call.getCalledOperand();
  auto *callee_type = call.getFunctionType();
  Vector<llvm::Value *> args(call.arg_begin(), call.arg_end());

  // Transform the program, s.t. we check the function pointer for each possible
  // callee, and make a direct call for each.
  llvm::IRBuilder<> builder(&call);

  // If the call has a return value, we need to create phis.
  auto *ret_type = callee_type->getReturnType();
  bool create_phi = ret_type and not ret_type->isVoidTy();

  // Create a nested series of if-then-else blocks.
  llvm::PHINode *phi = NULL;
  llvm::Instruction *then_term = NULL, *else_term = &call;
  for (auto *func : possible_callees) {
    // Build before the else block terminator.
    builder.SetInsertPoint(else_term);

    // Construct an equality check between the callee and the possible callee.
    auto *cmp = builder.CreateICmpEQ(callee, func, "defunc.check.");

    // If this is the last callee, and the callee is known to be in the set of
    // possible callees, don't insert and else block at the end.
    bool create_else = possibly_unknown or func != possible_callees.back();

    // Construct a PHI node, if needed.
    llvm::PHINode *split_phi = NULL;
    if (create_phi) {
      split_phi = builder.CreatePHI(ret_type, /* # reserved */ 2);
    }

    // Insert control flow.
    auto *split_before = split_phi ? split_phi : else_term;
    if (create_else) {
      // Create if-then-else.
      llvm::SplitBlockAndInsertIfThenElse(cmp,
                                          split_before,
                                          &then_term,
                                          &else_term);
    } else {
      // If we don't need an else block at this nesting level, the parent else
      // block is this nesting level's then block.
      then_term = else_term;
    }

    // Create a direct call in the then block.
    auto *new_call = cast<llvm::CallBase>(call.clone());
    new_call->setCalledFunction(func);
    new_call->insertBefore(then_term);

    if (create_phi) {
      // Patch up the PHI at the current nesting level.
      split_phi->addIncoming(new_call, new_call->getParent());

      // Patch up the PHI at the parent nesting level.
      if (phi) {
        phi->addIncoming(split_phi, split_phi->getParent());
      } else {
        // If this is the first PHI we create, replace the call with it.
        call.replaceAllUsesWith(split_phi);
      }

      // Update the parent PHI for the next iteration.
      phi = split_phi;
    }

    // Continue iterating in the else block...
  }

  // Move the original call into the final else block, if callee is possibly
  // unknown.
  if (possibly_unknown) {
    call.moveBefore(else_term);
  }

  // Update the final PHI, if needed.
  if (create_phi and phi) {
    phi->addIncoming(&call, call.getParent());
  }

  return true;
}

bool defunctionalize(llvm::CallBase &call, bool possibly_unknown) {

  // If this is a direct call, no need to modify it!
  if (not call.isIndirectCall()) {
    return false;
  }

  println("DEFUNC ", call);

  // Unpack the call.
  auto *callee_type = call.getFunctionType();

  // Fetch the set of all possible callees.
  Vector<llvm::Function *> possible_callees = {};
  auto &module =
      MEMOIR_SANITIZE(call.getModule(), "Call does not belong to a module!");
  for (auto &func : module) {
    // Check that the function type matches the caller type.
    auto *func_type = func.getFunctionType();
    if (func_type != callee_type) {
      continue;
    }

    // If the function's address is never taken, then this won't be called here.
    // TODO: Extend this to use alias analysis, if provided.
    if (not address_taken(func)) {
      continue;
    }

    // Mark the function as a possible callee.
    possible_callees.push_back(&func);
  }

  defunctionalize(call, possibly_unknown, possible_callees);
}

bool defunctionalize(llvm::Function &F) {
  bool modified = false;

  Set<llvm::CallBase *> calls = {};
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *call = dyn_cast<llvm::CallBase>(&I)) {
        calls.insert(call);
      }
    }
  }

  // For each call, determine if its callee is possibly unknown.
  for (auto *call : calls) {
    modified |= defunctionalize(*call);
  }

  if (modified) {
    println(F);
  }

  return modified;
}

bool defunctionalize(llvm::Module &M) {
  bool modified = false;

  for (auto &F : M) {
    modified |= defunctionalize(F);
  }

  return modified;
}

llvm::PreservedAnalyses DefunctionalizePass::run(
    llvm::Function &F,
    llvm::FunctionAnalysisManager &FAM) {

  auto modified = defunctionalize(F);

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

} // namespace llvm::memoir
