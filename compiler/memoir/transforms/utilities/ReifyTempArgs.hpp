#ifndef MEMOIR_TRANSFORMS_UTILITIES_REIFYTEMPARGS_H
#define MEMOIR_TRANSFORMS_UTILITIES_REIFYTEMPARGS_H

// LLVM
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/Cloning.h"

// MEMOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

/**
 * Convert tempargs in the given LLVM module into formal arguments to the
 * function.
 *
 * @returns TRUE if the module was modified.
 */
bool reify_tempargs(llvm::Module &M) {
  bool changed = false;

  // Collect all of the loads and stores marked as temporary arguments.
  ordered_map<llvm::Function *, vector<llvm::LoadInst *>> temp_loads = {};
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        // Filter out irrelevant instruction types.
        auto *load = dyn_cast<llvm::LoadInst>(&I);
        if (not load) {
          continue;
        }

        // Check for temporary argument metadata.
        auto temp_arg_metadata = Metadata::get<TempArgumentMetadata>(I);
        if (not temp_arg_metadata) {
          continue;
        }

        // Save the temporary arg load.
        temp_loads[&F].push_back(load);
      }
    }
  }

  // For each function with temporary arguments, clone the function with the
  // additional arguments.
  map<llvm::GlobalVariable *, llvm::Argument *> global_to_arg = {};
  map<llvm::Function *, llvm::Function *> function_clones = {};
  set<llvm::Instruction *> to_cleanup = {};
  for (auto &[func, loads] : temp_loads) {
    // Fetch the old function type.
    auto *old_func_type = func->getFunctionType();

    // Construct the new function type.
    auto *return_type = old_func_type->getReturnType();
    vector<llvm::Type *> param_types(old_func_type->param_begin(),
                                     old_func_type->param_end());
    for (auto *load : loads) {
      // Add the parameter type for the load.
      auto *load_type = load->getType();
      param_types.push_back(load_type);
    }
    auto *new_func_type = llvm::FunctionType::get(return_type,
                                                  param_types,
                                                  /* variadic? */ false);

    // Create the new function.
    auto &new_func = MEMOIR_SANITIZE(llvm::Function::Create(new_func_type,
                                                            func->getLinkage(),
                                                            func->getName(),
                                                            M),
                                     "Failed to create function clone.");

    function_clones[func] = &new_func;

    // Clone the function into a new function.
    llvm::ValueToValueMapTy vmap;
    unsigned arg_no = 0;
    for (auto &old_arg : func->args()) {
      auto *new_arg = new_func.arg_begin() + arg_no++;
      vmap.insert({ &old_arg, new_arg });
    }
    llvm::SmallVector<llvm::ReturnInst *, 8> returns;
    llvm::CloneFunctionInto(&new_func,
                            func,
                            vmap,
                            llvm::CloneFunctionChangeType::LocalChangesOnly,
                            returns);

    new_func.takeName(func);

    // Replace each of the old temporary arguments with its new argument. In the
    // process, create a mapping from temparg global to the formal parameter.
    for (auto *load : loads) {

      // Fetch the new state.
      auto *new_load = cast<llvm::LoadInst>(&*vmap[load]);
      auto *new_arg = new_func.getArg(arg_no++);

      // Save the corresponding global variable.
      auto *ptr = load->getPointerOperand();
      auto *global = dyn_cast<llvm::GlobalVariable>(ptr);
      MEMOIR_NULL_CHECK(
          global,
          "Temporary argument load has non-global pointer operand.");
      global_to_arg[global] = new_arg;

      // Replace the load with the argument.
      new_load->replaceAllUsesWith(new_arg);

      to_cleanup.insert(new_load);
    }
  }

  // Find each of the tempargs stores and patch the call site.
  ordered_map<llvm::CallBase *, set<llvm::StoreInst *>> temp_stores = {};
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    // Skip functions that have been cloned.
    if (function_clones.count(&F) > 0) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        // Filter out irrelevant instruction types.
        auto *store = dyn_cast<llvm::StoreInst>(&I);
        if (not store) {
          continue;
        }

        // Check for temporary argument metadata.
        auto temp_arg_metadata = Metadata::get<TempArgumentMetadata>(I);
        if (not temp_arg_metadata) {
          continue;
        }

        if (auto *store = dyn_cast<llvm::StoreInst>(&I)) {
          // Find the corresponding call.
          llvm::CallBase *call = nullptr;
          for (auto *curr = store->getNextNode(); curr != nullptr;
               curr = curr->getNextNode()) {
            if (into<FoldInst>(curr) or not into<MemOIRInst>(curr)) {
              call = dyn_cast<llvm::CallBase>(curr);
              if (call) {
                break;
              }
            }
          }
          if (not call) {
            MEMOIR_UNREACHABLE("Failed to find call for temp arg.");
          }

          // Save the temporary arg store.
          temp_stores[call].insert(store);
        }
      }
    }
  }

  // Patch each call with the new arguments.
  for (auto &[call, stores] : temp_stores) {

    // Check if the call is a fold.
    auto *fold = into<FoldInst>(call);

    // Fetch the called function.
    auto &called_func =
        (not fold)
            ? MEMOIR_SANITIZE(call->getCalledFunction(),
                              "Temporary argument used in indirect call!")
            : fold->getBody();

    // Fetch the new function.
    auto found_clone = function_clones.find(&called_func);
    if (found_clone == function_clones.end()) {
      println(*call->getFunction());
      MEMOIR_UNREACHABLE("No clone for ", called_func.getName());
    }
    auto *cloned_func = found_clone->second;

    if (fold) {
      // Construct the list of arguments, initialize with the current closed
      // arguments.
      // [ closed... ]
      vector<llvm::Value *> new_closed(fold->closed_begin(),
                                       fold->closed_end());

      // Allocate space for the new arguments.
      // [ closed..., null, ..., null]
      new_closed.insert(new_closed.end(), stores.size(), nullptr);

      // Then we will replace each of the stores.
      // [ closed..., store0, ..., storeN ]
      unsigned arg_offset = (not fold->getElementArgument()) ? 2 : 3;
      for (auto *store : stores) {
        auto *ptr = store->getPointerOperand();
        auto *global = dyn_cast<llvm::GlobalVariable>(ptr);
        MEMOIR_NULL_CHECK(global, "Temporary argument store to non-global.");

        auto found = global_to_arg.find(global);
        if (found == global_to_arg.end()) {
          MEMOIR_UNREACHABLE(
              "Temporary argument store with no matching argument.");
        }

        auto *arg = found->second;
        auto closed_no = arg->getArgNo() - arg_offset;

        new_closed[closed_no] = store->getValueOperand();

        to_cleanup.insert(store);
      }

      // Construct a new fold.
      MemOIRBuilder builder(call);
      vector<llvm::Value *> indices(fold->indices_begin(), fold->indices_end());
      auto *new_fold = builder.CreateFoldInst(fold->getKind(),
                                              cloned_func,
                                              &fold->getInitial(),
                                              &fold->getObject(),
                                              indices,
                                              new_closed);
      auto &new_call = new_fold->getCallInst();
      new_call.copyMetadata(*call);

      // Replace the old call with this one.
      if (call->hasNUsesOrMore(1)) {
        call->replaceAllUsesWith(&new_call);
      }

      auto *parent_func = call->getFunction();
      if (llvm::verifyFunction(*parent_func, &llvm::errs())) {
        MEMOIR_UNREACHABLE("Failed to verify ", parent_func->getName());
      }

    } else {
      // Construct the list of arguments.
      vector<llvm::Value *> arguments(cloned_func->arg_size(), nullptr);

      // First, copy over the current list of arguments.
      auto arg_idx = 0;
      for (auto &orig_arg_use : call->args()) {
        auto *orig_arg = orig_arg_use.get();
        arguments[arg_idx++] = orig_arg;
      }

      // Then we will replace each of the stores.
      for (auto *store : stores) {
        auto *ptr = store->getPointerOperand();
        auto *global = dyn_cast<llvm::GlobalVariable>(ptr);
        MEMOIR_NULL_CHECK(global, "Temporary argument store to non-global.");

        auto found = global_to_arg.find(global);
        if (found == global_to_arg.end()) {
          MEMOIR_UNREACHABLE(
              "Temporary argument store with no matching argument.");
        }

        auto *arg = found->second;
        auto arg_no = arg->getArgNo();

        arguments[arg_no] = store->getValueOperand();

        to_cleanup.insert(store);
      }

      // Construct a new call.
      MemOIRBuilder builder(call);
      auto *new_call = builder.CreateCall(cloned_func->getFunctionType(),
                                          cloned_func,
                                          arguments);
      new_call->copyMetadata(*call);

      // Replace the old call with this one.
      if (call->hasNUsesOrMore(1)) {
        call->replaceAllUsesWith(new_call);
      }

      // Find any RetPHIs that need to be replaced.
      for (auto *curr = call->getNextNode(); curr != nullptr;
           curr = curr->getNextNode()) {
        if (auto *ret_phi = into<RetPHIInst>(curr)) {
          ret_phi->getCalledOperandAsUse().set(cloned_func);
        } else if (isa<llvm::CallBase>(curr)) {
          // If we see another call, stop searching.
          break;
        }
      }
    }

    // Mark the call for removal.
    to_cleanup.insert(call);
  }

  // Remove all of the instructions marked for cleanup.
  for (auto *inst : to_cleanup) {
    inst->eraseFromParent();
  }

  // Remove all of the old functions.
  for (auto [old_func, _] : function_clones) {
    old_func->deleteBody();
    old_func->eraseFromParent();
  }

  return changed;
}

} // namespace llvm::memoir

#endif // MEMOIR_TRANSFORMS_UTILITIES_REIFYTEMPARGS_H
