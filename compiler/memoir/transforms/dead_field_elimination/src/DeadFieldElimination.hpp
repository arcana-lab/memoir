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
 * This class eliminates trivially dead field.
 *
 * Author(s): Tommy McMichen
 * Created: August 25, 2023
 */

namespace llvm::memoir {

class DeadFieldElimination
  : InstVisitor<DeadFieldElimination, set<llvm::Value *>> {
  friend class InstVisitor<DeadFieldElimination, set<llvm::Value *>>;
  friend class llvm::InstVisitor<DeadFieldElimination, set<llvm::Value *>>;

  using FieldToAccessMapTy = map<unsigned, set<AccessInst *>>;
  using LiveFieldMapTy = map<StructType *, FieldToAccessMapTy>;
  using StructTypeSetTy = set<StructType *>;
  using StructSetTy = set<llvm::Value *>;
  using StructToTypeMapTy = map<StructType *, StructSetTy>;

public:
  bool transformed;

  DeadFieldElimination(llvm::Module &M) {
    // Run dead collection elimination.
    this->transformed = this->run(M);
  }

protected:
  // Top-level driver.
  bool run(llvm::Module &M) {
    return this->transform(this->analyze(M));
  }

  // Helpers
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

  static inline void analyze_value(llvm::Value &V,
                                   StructToTypeMapTy &structs_of_type,
                                   StructTypeSetTy &escaped_types) {
    // Check if this value is a struct type.
    if (!Type::value_is_struct_type(V)) {
      return;
    }

    infoln("Found struct: ", V);

    // Get the type of this struct.
    auto *type = TypeAnalysis::analyze(V);
    auto *struct_type = dyn_cast_or_null<StructType>(type);
    MEMOIR_NULL_CHECK(struct_type,
                      "Could not determine the StructType of a struct value!");

    // Check if the struct escapes.
    if (value_escapes(V)) {
      infoln("Value escapes! ", V);
      escaped_types.insert(struct_type);
      return;
    }

    // Otherwise, record this struct value.
    structs_of_type[struct_type].insert(&V);

    return;
  }

  // Analysis
  LiveFieldMapTy analyze(llvm::Module &M) {
    LiveFieldMapTy live_fields = {};

    // Find all structs.
    StructToTypeMapTy structs_of_type = {};
    StructTypeSetTy escaped_types = {};

    // For each function:
    for (auto &F : M) {
      // Ignore function declarations.
      if (F.empty()) {
        continue;
      }

      // Check the arguments.
      for (auto &A : F.args()) {
        analyze_value(A, structs_of_type, escaped_types);
      }

      // Check the function.
      for (auto &BB : F) {
        for (auto &I : BB) {
          analyze_value(I, structs_of_type, escaped_types);
        }
      }
    }

    debugln("Got structs and their types.");

    // For each struct type:
    for (auto const &[struct_type, struct_values] : structs_of_type) {
      auto &index_to_access_map = live_fields[struct_type];
      // Inspect the values of its type.
      for (auto *value : struct_values) {
        // Get all users of this values.
        for (auto *user : value->users()) {
          // Only need to get direct users, we've already gathered each named
          // variable so PHIs will be handled on their own.
          auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
          if (user_as_inst == nullptr) {
            continue;
          }

          // Only need to inspect memoir instructions.
          auto *user_as_memoir = MemOIRInst::get(*user_as_inst);

          // Inspect struct accesses.
          if (auto *user_as_access =
                  dyn_cast_or_null<AccessInst>(user_as_memoir)) {

            // Get the field index.
            unsigned field_index;
            if (auto *user_as_struct_read =
                    dyn_cast<StructReadInst>(user_as_access)) {
              field_index = user_as_struct_read->getFieldIndex();
            } else if (auto *user_as_struct_write =
                           dyn_cast_or_null<StructWriteInst>(user_as_access)) {
              field_index = user_as_struct_write->getFieldIndex();
            } else if (auto *user_as_struct_get =
                           dyn_cast_or_null<StructGetInst>(user_as_access)) {
              field_index = user_as_struct_get->getFieldIndex();
            }

            // Register the field index as being alive with this access.
            index_to_access_map[field_index].insert(user_as_access);
          }

          // If it's not a struct access, we have nothing to look at.
        }
      }
    }

    debugln("Done analyzing");

    return live_fields;
  }

  // Transformation
  bool transform(LiveFieldMapTy &&live_fields) {
    bool transformed = false;

    set<llvm::Instruction *> instructions_to_delete = {};

    // For each struct type:
    for (auto const &[struct_type, field_index_to_accesses] : live_fields) {
      debugln("Transforming ", *struct_type);

      // Get the type definition as an instruction.
      auto &type_definition = struct_type->getDefinition();
      set<unsigned> fields_to_remove = {};

      // For each field in the struct definition:
      unsigned num_fields = struct_type->getNumFields();
      debugln(" # fields = ", num_fields);
      vector<llvm::Value *> fields_after_removal; // this is in reverse order.
      fields_after_removal.reserve(num_fields + 2);
      for (int16_t field_idx = num_fields - 1; field_idx >= 0; field_idx--) {
        // debugln("  inspecting index ", field_idx);

        // If the field is alive, we have nothing to do.
        auto found_field = field_index_to_accesses.find(field_idx);
        if (found_field != field_index_to_accesses.end()) {
          fields_after_removal.push_back(
              &type_definition.getFieldTypeOperand(field_idx));
          continue;
        }

        transformed |= true;

        // Otherwise, we need to:
        //  1. mark this field for removal.
        fields_to_remove.insert(field_idx);

        //  2. update the field index of all accesses.
        for (unsigned orig_field_idx = field_idx + 1;
             orig_field_idx < num_fields;
             orig_field_idx++) {
          debugln("  updating index ", orig_field_idx);

          auto found_orig_field = field_index_to_accesses.find(orig_field_idx);
          if (found_orig_field == field_index_to_accesses.end()) {
            continue;
          }

          for (auto *access : found_orig_field->second) {
            unsigned access_field_index;
            llvm::Use *access_field_index_use;
            if (auto *struct_read = dyn_cast<StructReadInst>(access)) {
              access_field_index = struct_read->getFieldIndex();
              access_field_index_use =
                  &struct_read->getFieldIndexOperandAsUse();
            } else if (auto *struct_write = dyn_cast<StructWriteInst>(access)) {
              access_field_index = struct_write->getFieldIndex();
              access_field_index_use =
                  &struct_write->getFieldIndexOperandAsUse();
            } else if (auto *struct_get = dyn_cast<StructGetInst>(access)) {
              access_field_index = struct_get->getFieldIndex();
              access_field_index_use = &struct_get->getFieldIndexOperandAsUse();
            } else {
              MEMOIR_UNREACHABLE(
                  "We have a field access that is no longer a Struct{Read,Write,Get}Inst!");
            }

            // If the field index is less than that of the field being
            // deleted, do nothing.
            if (access_field_index < field_idx) {
              continue;
            }

            // Get the constant for field index-1.
            auto *field_index_type = access_field_index_use->get()->getType();
            auto *new_field_index_value =
                llvm::ConstantInt::get(field_index_type,
                                       access_field_index - 1);

            // Replace the field index in the access.
            access_field_index_use->set(new_field_index_value);
          }
        }
      }

      // If there are fields to remove, let's update the struct type
      // definition.
      if (fields_to_remove.size() > 0) {
        // Get information about the type definition.
        auto &type_definition_as_call = type_definition.getCallInst();

        // Get the new num fields constant integer.
        auto &num_fields_as_value = type_definition.getNumberOfFieldsOperand();
        auto *field_index_type = num_fields_as_value.getType();

        auto new_num_fields = num_fields - fields_to_remove.size();
        auto *new_num_fields_value =
            llvm::ConstantInt::get(field_index_type, new_num_fields);
        fields_after_removal.push_back(new_num_fields_value);

        // Get the name of the struct type.
        auto &type_name = type_definition.getNameOperand();
        fields_after_removal.push_back(&type_name);

        // Construct a new call to replace this one.
        auto *function_type = type_definition_as_call.getFunctionType();
        auto *function = type_definition_as_call.getCalledFunction();
        auto *new_type_definition = CallInst::Create(
            function_type,
            function,
            vector<llvm::Value *>(fields_after_removal.rbegin(),
                                  fields_after_removal.rend()),
            /*NameStr=*/"dfe.",
            /*InsertBefore=*/&type_definition_as_call);

        // Replace the old call with this one.
        type_definition_as_call.replaceAllUsesWith(new_type_definition);

        // Mark old one for cleanup.
        instructions_to_delete.insert(&type_definition_as_call);
      }
    }

    // Cleanup instructions.
    for (auto *inst : instructions_to_delete) {
      inst->eraseFromParent();
    }

    return transformed;
  }
};

} // namespace llvm::memoir

#endif
