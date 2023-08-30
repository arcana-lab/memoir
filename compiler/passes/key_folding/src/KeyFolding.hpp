#ifndef MEMOIR_DEADCOLLECTIONELIMINATION_H
#define MEMOIR_DEADCOLLECTIONELIMINATION_H
#pragma once

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/CFG.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Function.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

/*
 * This class folds the key-space of assoc collections onto the index-space of a
 * sequence when possible..
 *
 * Author(s): Tommy McMichen
 * Created: August 28, 2023
 */

namespace llvm::memoir {

class KeyFolding {

public:
  bool transformed;

  KeyFolding(llvm::Module &M) {
    // Run dead collection elimination.
    this->transformed = this->run(M);
  }

protected:
  // Top-level driver.
  bool run(llvm::Module &M) {
    return this->transform(this->analyze(M));
  }

  // Analysis Types.
  struct KeyFoldingInfo {
    map<AssocArrayAllocInst *, CollectionAllocInst *> key_folds;
    map<AssocArrayAllocInst *, map<AccessInst *, llvm::Use *>> assoc_accesses;
    map<llvm::Use *, llvm::Value *> key_to_index;
  };

  // Analysis Helpers.
  static inline bool value_escapes(llvm::Value &V) {
    for (auto *user : V.users()) {
      if (auto *user_as_call = dyn_cast_or_null<llvm::CallBase>(user)) {
        // If the user is a memoir instruction, continue.
        if (auto *user_as_memoir = MemOIRInst::get(*user_as_call)) {
          continue;
        }

        // Get the called function.
        auto *called_function = user_as_call->getCalledFunction();

        // If it is an indirect call, then the value escapes.
        // TODO: improve this by using NOELLE's complete call graph
        if (called_function == nullptr) {
          return true;
        }

        // If it is empty, then the value escapes.
        if (called_function->empty()) {
          return true;
        }
      }
    }

    return false;
  }

  static inline set<llvm::Value *> gather_returned_values(llvm::Function &F) {
    set<llvm::Value *> returned_values = {};

    for (auto &BB : F) {
      auto *terminator = BB.getTerminator();
      if (auto *return_inst = dyn_cast<llvm::ReturnInst>(terminator)) {
        auto *return_value = return_inst->getReturnValue();
        returned_values.insert(return_value);
      }
    }

    return returned_values;
  }

  static inline set<llvm::Value *> gather_possible_arguments(
      llvm::Module &M,
      llvm::Argument &A) {
    set<llvm::Value *> possible_arguments = {};

    auto *arg_function = A.getParent();

    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *call = dyn_cast<llvm::CallBase>(&I)) {
            // If this is a direct call, check if it calls the function we care
            // about. If it does, stash the passed argument.
            if (auto *called_function = call->getCalledFunction()) {
              if (called_function == &F) {
                auto *passed_argument =
                    (call->arg_begin() + A.getArgNo())->get();
                possible_arguments.insert(passed_argument);
              }
            }
            // Otherwise, this is an indirect call. Check if it _may_ call the
            // function, if so, stash the passed argument.
            else {
              auto *function_type = call->getFunctionType();
              if (function_type == F.getFunctionType()) {
                auto *passed_argument =
                    (call->arg_begin() + A.getArgNo())->get();
                possible_arguments.insert(passed_argument);
              }
            }
          }
        }
      }
    }

    return possible_arguments;
  }

  // Analysis Driver.
  KeyFoldingInfo analyze(llvm::Module &M) {
    KeyFoldingInfo info;
    auto &key_folds = info.key_folds;
    auto &assoc_accesses = info.assoc_accesses;
    auto &key_to_index = info.key_to_index;

    // Find all associative and sequential collection allocations.
    map<AssocArrayAllocInst *, AssocArrayType *> assoc_allocations = {};
    map<SequenceAllocInst *, SequenceType *> seq_allocations = {};

    // For each function:
    for (auto &F : M) {
      // Ignore function declarations.
      if (F.empty()) {
        continue;
      }

      // Check the function.
      for (auto &BB : F) {
        for (auto &I : BB) {
          auto *memoir_inst = MemOIRInst::get(I);
          if (memoir_inst == nullptr) {
            continue;
          }

          if (auto *seq_alloc = dyn_cast<SequenceAllocInst>(memoir_inst)) {
            auto *seq_type =
                dyn_cast<SequenceType>(&seq_alloc->getCollectionType());
            MEMOIR_NULL_CHECK(seq_type,
                              "Sequence allocation is not of sequence type!");

            seq_allocations[seq_alloc] = seq_type;

            debugln("Found Sequence Alloc: ", *seq_alloc);

          } else if (auto *assoc_alloc =
                         dyn_cast<AssocArrayAllocInst>(memoir_inst)) {
            auto *assoc_type =
                dyn_cast<AssocArrayType>(&assoc_alloc->getCollectionType());
            MEMOIR_NULL_CHECK(assoc_type,
                              "Assoc allocation is not of assoc type!");

            assoc_allocations[assoc_alloc] = assoc_type;

            debugln("Found Assoc Alloc: ", assoc_alloc);
          }
        }
      }
    }

    debugln("Got assoc and seq allocations.");

    // Next, we need to check if there is any assoc-sequence pair for which all
    // accesses to the assoc are via references to the elements of the sequence.

    // For each assoc allocation:
    for (auto const &[assoc_alloc, assoc_type] : assoc_allocations) {
      debugln("Visiting ", *assoc_alloc);

      // Check that the assoc type has a reference type for its key.
      auto &key_type = assoc_type->getKeyType();
      auto *key_ref_type = dyn_cast<ReferenceType>(&key_type);
      if (key_ref_type == nullptr) {
        debugln("  key is not a reference");
        continue;
      }
      auto &referenced_type = key_ref_type->getReferencedType();
      auto *referenced_struct_type = dyn_cast<StructType>(&referenced_type);
      if (referenced_struct_type == nullptr) {
        debugln("  key is not a struct reference");
        continue;
      }

      // Get the LLVM representation of the assoc alloc.
      auto &llvm_assoc_alloc = assoc_alloc->getCallInst();

      // Inspect each access to this collection, recording the source of their
      // key value for each.
      auto &assoc_alloc_accesses = assoc_accesses[assoc_alloc];

      set<llvm::Value *> visited = {};
      vector<llvm::Value *> worklist = {};
      worklist.push_back(&llvm_assoc_alloc);
      while (!worklist.empty()) {
        // Pop an item of the worklist.
        auto *workitem = worklist.back();
        worklist.pop_back();

        // Check that this item hasn't been visited.
        if (visited.find(workitem) != visited.end()) {
          continue;
        } else {
          visited.insert(workitem);
        }

        // Iterate over the uses of this item.
        for (auto &use : workitem->uses()) {
          auto *user = use.getUser();

          // Check that the user is an instruction. If not skip it.
          auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
          if (user_as_inst == nullptr) {
            continue;
          }

          // Handle memoir instructions.
          if (auto *user_as_memoir = MemOIRInst::get(*user_as_inst)) {
            // If this is an assoc access instruction, fetch the key used to
            // access and stash them.
            if (auto *assoc_read = dyn_cast<AssocReadInst>(user_as_memoir)) {
              auto &key_use = assoc_read->getKeyOperandAsUse();
              assoc_alloc_accesses[assoc_read] = &key_use;
            } else if (auto *assoc_write =
                           dyn_cast<AssocWriteInst>(user_as_memoir)) {
              auto &key_use = assoc_write->getKeyOperandAsUse();
              assoc_alloc_accesses[assoc_write] = &key_use;
            } else if (auto *assoc_get =
                           dyn_cast<AssocGetInst>(user_as_memoir)) {
              auto &key_use = assoc_get->getKeyOperandAsUse();
              assoc_alloc_accesses[assoc_get] = &key_use;
            }

            // If this is a Use/DefPHI, iterate on its resultant.
            else if (auto *use_phi = dyn_cast<UsePHIInst>(user_as_memoir)) {
              worklist.push_back(&use_phi->getCallInst());
            } else if (auto *def_phi = dyn_cast<DefPHIInst>(user_as_memoir)) {
              worklist.push_back(&def_phi->getCallInst());
            }
          }

          // Handle PHI Nodes, iterating on the resultant.
          if (auto *use_as_phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
            worklist.push_back(use_as_phi);
          }

          // Handle call instructions, iterating on the argument corresponding
          // to this use.
          if (auto *user_as_call = dyn_cast<llvm::CallBase>(user_as_inst)) {
            auto argument_number = use.getOperandNo();

            // Fetch the called function, if this is a direct call, handle it.
            auto *called_function = user_as_call->getCalledFunction();
            if (called_function != nullptr) {
              auto *argument = called_function->arg_begin() + argument_number;
              worklist.push_back(argument);
              continue;
            }

            // If this is an indirect call, conservatively grab every function
            // that _may_ be called.
            auto *function_type = user_as_call->getFunctionType();
            MEMOIR_NULL_CHECK(function_type,
                              "CallBase has NULL function type!");
            for (auto &F : M) {
              auto *other_function_type = F.getFunctionType();

              // If this function _could_ be called (becuase it shares a
              // function type with the call), grab its argument and add it to
              // the worklist.
              if (other_function_type == function_type) {
                auto *argument = F.arg_begin() + argument_number;
                worklist.push_back(argument);
                continue;
              }
            }
          }
        }
      }

      // Next, we will check each of the keys used to access this collection to
      // see if it is _always_ derived from the same collection.
      bool unknown_behavior = false;
      set<llvm::Value *> collections_accessed = {};
      for (auto const &[assoc_access, assoc_key] : assoc_alloc_accesses) {
        println("  access: ", *assoc_access);

        // Check if the key is an instruction.
        auto *key_as_inst = dyn_cast<llvm::Instruction>(assoc_key->get());
        if (key_as_inst == nullptr) {
          unknown_behavior = true;
          break;
        }

        // Check if the key is a memoir instruction.
        if (auto *key_as_memoir = MemOIRInst::get(*key_as_inst)) {
          // If the key is the result of an IndexGetInst, record the sequence
          // accessed and the mapping from this key to the index accessed.
          if (auto *key_as_index_get = dyn_cast<IndexGetInst>(key_as_memoir)) {
            // Fetch information from the IndexGetInst.
            auto &sequence_accessed = key_as_index_get->getObjectOperand();
            auto &index_accessed = key_as_index_get->getIndexOfDimension(0);

            // Record the sequence variable that was accessed along with the
            // key-to-index mapping.
            collections_accessed.insert(&sequence_accessed);
            key_to_index[assoc_key] = &index_accessed;

            // Keep on chugging.
            continue;
          }

          // If the key is the result of an AssocGetInst, record the sequence
          // accessed and the mapping from this key to the key accessed.
          if (auto *key_as_assoc_get = dyn_cast<AssocGetInst>(key_as_memoir)) {
            // Fetch information from the IndexGetInst.
            auto &assoc_accessed = key_as_assoc_get->getObjectOperand();
            auto &key_accessed = key_as_assoc_get->getKeyOperand();

            // Record the sequence variable that was accessed along with the
            // key-to-index mapping.
            collections_accessed.insert(&assoc_accessed);
            key_to_index[assoc_key] = &key_accessed;

            // Keep on chugging.
            continue;
          }
        }

        // TODO: make this analysis more powerful by iterating along PHIs and
        // building ValueExpressions for the key-to-index mapping.

        // Otherwise, record this as unknown behavior and leave.
        unknown_behavior = true;
        break;
      }

      // If we found behavior that we couldn't analyze, then don't key fold
      // here.
      if (unknown_behavior) {
        debugln("  found unknown behavior, skipping.");
        continue;
      }

      // Check that all collections accessed _must_ be the same allocation.
      set<CollectionAllocInst *> collections_referenced = {};

      visited.clear();
      worklist.clear();
      worklist.insert(worklist.end(),
                      collections_accessed.begin(),
                      collections_accessed.end());
      while (!worklist.empty()) {
        // Pop an item of the worklist.
        auto *workitem = worklist.back();
        worklist.pop_back();

        // Check that this item hasn't been visited.
        if (visited.find(workitem) != visited.end()) {
          continue;
        } else {
          visited.insert(workitem);
        }

        // If the workitem is an instruction, handle each case in turn.
        if (auto *inst = dyn_cast<llvm::Instruction>(workitem)) {
          if (auto *memoir_inst = MemOIRInst::get(*inst)) {
            if (auto *seq_alloc = dyn_cast<SequenceAllocInst>(memoir_inst)) {
              collections_referenced.insert(seq_alloc);
            } else if (auto *assoc_alloc =
                           dyn_cast<AssocArrayAllocInst>(memoir_inst)) {
              collections_referenced.insert(assoc_alloc);
            } else if (auto *use_phi = dyn_cast<UsePHIInst>(memoir_inst)) {
              worklist.push_back(&use_phi->getCallInst());
            } else if (auto *def_phi = dyn_cast<DefPHIInst>(memoir_inst)) {
              worklist.push_back(&def_phi->getCallInst());
            } else {
              // TODO: add handling for slice and join instructions, this is
              // tricky though and would be better served by insert/remove
              // operators.
            }
          } else if (auto *phi = dyn_cast<llvm::PHINode>(inst)) {
            worklist.push_back(phi);
          } else if (auto *call = dyn_cast<llvm::CallBase>(inst)) {
            // Round up all the possible callees and iterate on their returns.

            // Fetch the called function, if this is a direct call, handle it.
            auto *called_function = call->getCalledFunction();
            if (called_function != nullptr) {
              auto returned_values = gather_returned_values(*called_function);
              worklist.insert(worklist.end(),
                              returned_values.begin(),
                              returned_values.end());
              continue;
            }

            // If this is an indirect call, conservatively grab every function
            // that _may_ be called.
            auto *function_type = call->getFunctionType();
            MEMOIR_NULL_CHECK(function_type,
                              "CallBase has NULL function type!");
            for (auto &F : M) {
              auto *other_function_type = F.getFunctionType();

              // If this function _could_ be called (becuase it shares a
              // function type with the call), grab its argument and add it to
              // the worklist.
              if (other_function_type == function_type) {
                auto returned_values = gather_returned_values(F);
                worklist.insert(worklist.end(),
                                returned_values.begin(),
                                returned_values.end());
                continue;
              }
            }
          }
        } else if (auto *arg = dyn_cast<llvm::Argument>(workitem)) {
          // Round up all the possible callers and iterate on their arguments.
          auto possible_arguments = gather_possible_arguments(M, *arg);
          worklist.insert(worklist.end(),
                          possible_arguments.begin(),
                          possible_arguments.end());
        } else {
          collections_referenced.insert(nullptr);
        }
      }

      // If there is more than one sequence that may be referenced, skip this
      // assoc.
      if (collections_referenced.size() > 1) {
        debugln("  keys of assoc may reference more than one sequence");
        continue;
      } else if (collections_referenced.size() == 0) {
        debugln("  could not find any sequence allocations referenced.");
        continue;
      }

      // Otherwise, record the assoc-collection pair for transformation.
      auto *collection_alloc = *(collections_referenced.begin());
      if (collection_alloc == nullptr) {
        debugln("  found an unhandled case");
        continue;
      }
      key_folds[assoc_alloc] = collection_alloc;
    }

    debugln("Found the following key folding pairs.");

    for (auto const &[assoc_alloc, collection_alloc] : key_folds) {
      debugln("  ", *assoc_alloc);
      debugln("    <==>");
      debugln("  ", *collection_alloc);
      debugln();
    }

    return info;
  }

  // Transformation helpers.
  static llvm::Function *get_index_function(llvm::Function &F) {
    auto &M = MEMOIR_SANITIZE(F.getParent(), "Could not get function's module");
    auto name = F.getName().str();

    // Replace assoc with index.
    // "memoir__assoc"
    auto suffix = name.substr(13);
    auto replacement = "memoir__index" + suffix;
    debugln("index function is ", replacement);

    // Get this function from the module.
    return M.getFunction(replacement);
  }

  // Transformation driver.
  bool transform(KeyFoldingInfo info) {
    auto &key_folds = info.key_folds;
    auto &assoc_accesses = info.assoc_accesses;
    auto &key_to_index = info.key_to_index;

    // Track if we actually transformed the program.
    bool transformed = false;

    // Stash for any instructions that will be removed by the transformation.
    set<llvm::Instruction *> instructions_to_delete = {};

    // For key-fold pair.
    for (auto const &[assoc_alloc, collection_alloc] : key_folds) {
      if (auto *seq_alloc = dyn_cast<SequenceAllocInst>(collection_alloc)) {
        // Convert the assoc allocation to be a sequence allocation the same
        // size as the sequence allocation.
        auto &seq_size = seq_alloc->getSizeOperand();

        // Check if the size is available at the assoc allocation site.
        // NOTE: this is a little conservative to keep things simple: if the
        // size is not available at the assoc allocation site, then we will
        // _not_ perform key folding for this candidate.
        llvm::Value *allocation_size;

        // If the size is a constant, it is available.
        if (auto *size_as_const_int = dyn_cast<llvm::ConstantInt>(&seq_size)) {
          allocation_size = size_as_const_int;
        }
        // If the size is an argument of the function containing the assoc
        // allocation, it is available.
        else if (auto *size_as_argument = dyn_cast<llvm::Argument>(&seq_size)) {
          allocation_size = size_as_argument;
        }
        // If the size dominates the assoc allocation, it is available.
        else if (false) {
          debugln("Sequence size based on dominance is unimplemented");
          allocation_size = nullptr;
        }
        // Otherwise, the size is not available, skip this pair.
        else {
          allocation_size = nullptr;
        }

        // Check that we were able to find an allocation size.
        if (allocation_size == nullptr) {
          debugln("Sequence size is not available.");
          continue;
        }

        // Build a new sequence allocation.
        MemOIRBuilder builder(*assoc_alloc, /*InsertAfter=*/false);

        // The value operand _will_ be available, because we are inserting
        // immediately before the existing allocation.
        auto &elem_type = assoc_alloc->getValueOperand();

        auto *folded_sequence = builder.CreateSequenceAllocInst(&elem_type,
                                                                allocation_size,
                                                                "folded.");

        // Replace the assoc allocation with the folded one.
        assoc_alloc->getCallInst().replaceAllUsesWith(
            &folded_sequence->getCallInst());

        // Mark the old allocation for cleanup.
        instructions_to_delete.insert(&assoc_alloc->getCallInst());

        // For each access, convert it to be an indexed access using the
        // key-to-index map.
        auto &accesses = assoc_accesses[assoc_alloc];
        for (auto const &[access, key_use] : accesses) {
          // Fetch the information about this access.
          auto &access_call = access->getCallInst();
          auto *access_function = access_call.getCalledFunction();
          MEMOIR_NULL_CHECK(access_function, "Access function is NULL!");

          // Get the index variant of this function.
          auto *replacement_function = get_index_function(*access_function);
          MEMOIR_NULL_CHECK(replacement_function,
                            "Couldn't get the index function");

          // Replace the called function.
          access_call.setCalledFunction(replacement_function);

          // Replace the key use with its corresponding index.
          // TODO: enhance this to materialize a ValueExpression.
          auto *folded_index = key_to_index[key_use];
          key_use->set(folded_index);
        }
      } else if (auto *fold_assoc_alloc =
                     dyn_cast<AssocArrayAllocInst>(collection_alloc)) {
        // Build a new assoc allocation.
        MemOIRBuilder builder(*assoc_alloc, /*InsertAfter=*/false);

        // Get the key type that we are folding onto.
        auto &key_type = fold_assoc_alloc->getKeyType();

        // The value type operand _will_ be available, because we are inserting
        // immediately before the existing allocation.
        auto &value_type = assoc_alloc->getValueOperand();

        auto *folded_alloc =
            builder.CreateAssocArrayAllocInst(key_type, &value_type, "folded.");

        // Replace the assoc allocation with the folded one.
        assoc_alloc->getCallInst().replaceAllUsesWith(
            &folded_alloc->getCallInst());

        // Mark the old allocation for cleanup.
        instructions_to_delete.insert(&assoc_alloc->getCallInst());

        // For each access, convert it to use the folded key.
        auto &accesses = assoc_accesses[assoc_alloc];
        for (auto const &[access, key_use] : accesses) {
          // Replace the key use with its corresponding index.
          // TODO: enhance this to materialize a ValueExpression.
          auto *folded_index = key_to_index[key_use];
          key_use->set(folded_index);
        }
      }
    }

    // Cleanup instructions.
    for (auto *inst : instructions_to_delete) {
      inst->eraseFromParent();
    }

    // Return, true if the program was transformed, false otherwise.
    return transformed;
  }
}; // namespace llvm::memoir

} // namespace llvm::memoir

#endif
