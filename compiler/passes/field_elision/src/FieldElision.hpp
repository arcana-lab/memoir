#ifndef MEMOIR_FIELDELISION_H
#define MEMOIR_FIELDELISION_H
#pragma once

#include <regex>

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/CallGraph.h"

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

#ifndef FIELD_ELISION_TEST
#  define FIELD_ELISION_TEST 1
#endif

/*
 * This class converts a field of a struct into an associative array.
 *
 * Author(s): Tommy McMichen
 * Created: August 25, 2023
 */

namespace llvm::memoir {

class FieldElision {
  using IndexSetTy = set<unsigned>;
  using ValueSetTy = set<llvm::Value *>;
  using InstSetTy = set<llvm::Instruction *>;
  using ArgSetTy = set<llvm::Argument *>;
  using AccessSetTy = set<AccessInst *>;
  using StructTypeSetTy = set<StructType *>;
  using TypeToStructsMapTy = map<StructType *, ValueSetTy>;
  using FieldsToInstsMapTy = map<StructType *, InstSetTy>;
  using FieldsToArgsMapTy = map<StructType *, ArgSetTy>;
  using TypeToFieldAccessesMapTy =
      map<StructType *, map<unsigned, AccessSetTy>>;
  using FieldsToAccessesMapTy = map<llvm::Function *, TypeToFieldAccessesMapTy>;

public:
  using FieldsToElideMapTy = map<StructType *, list<IndexSetTy>>;

  bool transformed;

  FieldElision(llvm::Module &M,
               llvm::CallGraph &CG,
               FieldsToElideMapTy &fields_to_elide) {
    // Run dead collection elimination.
    this->transformed = this->run(M, CG, fields_to_elide);
  }

protected:
  // Top-level driver.
  bool run(llvm::Module &M,
           llvm::CallGraph &CG,
           FieldsToElideMapTy &fields_to_elide) {
    return this->transform(M,
                           CG,
                           fields_to_elide,
                           this->analyze(M, fields_to_elide));
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
                                   FieldsToElideMapTy &fields_to_elide,
                                   TypeToStructsMapTy &structs_of_type,
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

// TODO: remove this after done with testing!
#if FIELD_ELISION_TEST
    auto &struct_fields_to_elide = fields_to_elide[struct_type];
    bool found = false;
    for (auto candidate : struct_fields_to_elide) {
      if (candidate.find(0) != candidate.end()) {
        found = true;
        break;
      }
    }
    if (!found) {
      fields_to_elide[struct_type].push_back({ 0 });
    }
#endif

    // Check if this struct is of a type we care about.
    if (fields_to_elide.find(struct_type) == fields_to_elide.end()) {
      return;
    }

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
  FieldsToAccessesMapTy analyze(llvm::Module &M,
                                FieldsToElideMapTy &fields_to_elide) {
    FieldsToAccessesMapTy fields_to_accesses = {};

    // Find all structs.
    map<llvm::Function *, TypeToStructsMapTy> structs_of_type = {};
    StructTypeSetTy escaped_types = {};

    // For each function:
    for (auto &F : M) {
      // Ignore function declarations.
      if (F.empty()) {
        continue;
      }

      auto &structs_of_type_in_function = structs_of_type[&F];

      // Gather struct arguments for the types we care about.
      for (auto &A : F.args()) {
        analyze_value(A,
                      fields_to_elide,
                      structs_of_type_in_function,
                      escaped_types);
      }

      // Gather struct variables for the types we care about.
      for (auto &BB : F) {
        for (auto &I : BB) {
          analyze_value(I,
                        fields_to_elide,
                        structs_of_type_in_function,
                        escaped_types);
        }
      }
    }

    infoln("Got structs and their types.");

    // Filter out the struct types that escaped.
    for (auto *escaped_type : escaped_types) {
      auto found = fields_to_elide.find(escaped_type);
      if (found != fields_to_elide.end()) {
        fields_to_elide.erase(found);
      }
    }

    // Filter out the struct types that had no variables.
    // for (auto it = fields_to_elide.begin(); it != fields_to_elide.end();
    // ++it)
    // {
    //   if (structs_of_type.find(it->first) == structs_of_type.end()) {
    //     it = fields_to_elide.erase(it);
    //   }
    // }

    infoln("Filtered escaped and unused struct types.");

    // For each function:
    for (auto &F : M) {
      auto &fields_to_accesses_for_function = fields_to_accesses[&F];

      // For each struct type that has a field elision candidate:
      for (auto const &[struct_type, elision_candidates] : fields_to_elide) {
        // Get the struct variables of this type.
        auto &struct_variables = structs_of_type[&F][struct_type];

        // Get the access map for this type.
        auto &index_to_access_map =
            fields_to_accesses_for_function[struct_type];

        // For each elision grouping:
        for (auto candidate : elision_candidates) {
          // Collect all accesses to update.
          for (auto *struct_variable : struct_variables) {
            // Get all users of this values.
            for (auto *user : struct_variable->users()) {
              // Only need to get direct users, we've already gathered each
              // named variable so PHIs will be handled on their own.
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
                               dyn_cast_or_null<StructWriteInst>(
                                   user_as_access)) {
                  field_index = user_as_struct_write->getFieldIndex();
                } else if (auto *user_as_struct_get =
                               dyn_cast_or_null<StructGetInst>(
                                   user_as_access)) {
                  field_index = user_as_struct_get->getFieldIndex();
                }

                // Register the field index as being alive with this access.
                index_to_access_map[field_index].insert(user_as_access);
              }

              // If it's not a struct access, we have nothing to look at.
            }
          }
        }
      }
    }

    infoln("Done analyzing");

    return fields_to_accesses;
  }

  // Transformation
  static Type &construct_elided_type(const StructType &struct_type,
                                     const IndexSetTy &indices_to_elide) {

    // If this is a single field, return the field's type.
    if (indices_to_elide.size() == 1) {
      auto field_index = *(indices_to_elide.begin());
      return struct_type.getFieldType(field_index);
    }

    // Otherwise, we need to create a struct type for it.
    // TODO: implement this :)
    MEMOIR_UNREACHABLE("Field elision with grouping is unimplemented.");
  }

  // TODO: make this extensible
  static llvm::Function *get_assoc_function(llvm::Function &F) {
    auto &M = MEMOIR_SANITIZE(F.getParent(), "Could not get function's module");
    auto name = F.getName().str();

    // Replace struct with assoc.
    // "memoir__struct"
    std::regex read_re("memoir__struct_read");
    auto replacement = std::regex_replace(name, read_re, "memoir__assoc_read");
    std::regex write_re("memoir__struct_write");
    replacement = std::regex_replace(replacement, write_re, "mut__assoc_write");

    infoln("assoc function is ", replacement);

    // Get this function from the module.
    return M.getFunction(replacement);
  }

  using UsedFieldsSetTy = multimap<StructType *, unsigned>;
  UsedFieldsSetTy &traverse_call_graph_node(
      map<llvm::Function *, UsedFieldsSetTy> &function_to_used_fields,
      FieldsToAccessesMapTy &fields_to_accesses,
      set<llvm::CallGraphNode *> visited,
      llvm::CallGraphNode &node) {
    // Mark this node as visited.
    visited.insert(&node);

    // Get node's information.
    auto *node_function = node.getFunction();

    // Get the set of used fields.
    auto &used_fields = function_to_used_fields[node_function];

    // Check for corner cases.
    if (node_function == nullptr) {
      // warnln("Node function is NULL!");
      return used_fields;
    } else if (FunctionNames::is_memoir_call(*node_function)) {
      return used_fields;
    } else if (node_function->empty()) {
      // warnln("Node function is defined externally");
      return used_fields;
    }

    debugln("CallGraph: Visiting ", node_function->getName());

    for (auto const &[function, callee_node] : node) {
      // Get the callee function.
      auto *callee_function = callee_node->getFunction();

      // If the callee is NULL, just ignore it. We have already checked for
      // structs escaping along indirect call edges. This may be naive, but
      // until we find a case that we need to handle it will remain that way.
      if (callee_function == nullptr) {
        warnln("Callee function is NULL within ", node_function->getName());
        continue;
      };

      // Check if this node has already visited.
      if (visited.find(callee_node) != visited.end()) {
        // Insert the elements from this node into our own used types.
        auto &callee_used_fields = function_to_used_fields[callee_function];
        for (auto const &[struct_type, field_index] : callee_used_fields) {
          insert_unique(used_fields, struct_type, field_index);
        }
        continue;
      }

      // If we haven't visited it, then visit the callee.
      auto &callee_used_fields =
          traverse_call_graph_node(function_to_used_fields,
                                   fields_to_accesses,
                                   visited,
                                   *callee_node);
      for (auto const &[struct_type, field_index] : callee_used_fields) {
        insert_unique(used_fields, struct_type, field_index);
      }
    }

    // Insert all fields used by this function.
    for (auto const &[struct_type, field_to_accesses] :
         fields_to_accesses[node_function]) {
      for (auto const &[field_index, accesses] : field_to_accesses) {
        if (accesses.size() > 0) {
          insert_unique(used_fields, struct_type, field_index);
        }
      }
    }

    return used_fields;
  }

  map<llvm::Function *, UsedFieldsSetTy> traverse_call_graph(
      llvm::CallGraph &CG,
      FieldsToAccessesMapTy &fields_to_accesses,
      const llvm::Function &entry_function) {

    map<llvm::Function *, UsedFieldsSetTy> function_to_used_fields = {};

    auto &entry_node = MEMOIR_SANITIZE(
        CG[&entry_function],
        "Could not get the CallGraphNode for the entry function.");

    set<llvm::CallGraphNode *> visited = {};
    traverse_call_graph_node(function_to_used_fields,
                             fields_to_accesses,
                             visited,
                             entry_node);

    return function_to_used_fields;
  }

  bool transform(llvm::Module &M,
                 llvm::CallGraph &CG,
                 FieldsToElideMapTy &fields_to_elide,
                 FieldsToAccessesMapTy fields_to_accesses) {
    bool transformed = false;

    set<llvm::Instruction *> instructions_to_delete = {};

    llvm::Type *llvm_collection_type = nullptr;

    // Now for the tricky bit.
    // We need to version all functions in the callgraph so that there is a
    // route from the entry function to each function where an access resides.
    auto &entry_function = MEMOIR_SANITIZE(
        M.getFunction("main"),
        "Could not get the main function, FieldElision assumes whole-program.");

    // Perform a post-order DFS traversal of the callgraph, collecting the
    // types that must be propagated along
    map<llvm::Function *, UsedFieldsSetTy> function_to_used_fields =
        traverse_call_graph(CG, fields_to_accesses, entry_function);

    // Collect the elided fields that will be added to each function's
    // list of parameters.
    // Map from function to a (struct_type, candidates[i])
    map<llvm::Function *, ordered_multimap<StructType *, size_t>>
        function_to_argument_order = {};
    for (auto const &[function, used_fields] : function_to_used_fields) {
      // Skip this if there are no used fields.
      if (used_fields.empty()) {
        continue;
      }

      debugln("Function ", function->getName(), " uses the following fields");
      auto &argument_order = function_to_argument_order[function];

      // Check for any fields that are elision candidates.
      for (auto const &[struct_type, candidates] : fields_to_elide) {
        auto used_fields_for_type = used_fields.equal_range(struct_type);

        for (auto it = used_fields_for_type.first;
             it != used_fields_for_type.second;
             ++it) {

          auto field_index = it->second;

          debugln("  Field ", field_index, " of ", struct_type->getName());

          size_t candidate_index = 0;
          for (auto candidate : candidates) {
            for (auto candidate_field_index : candidate) {
              if (field_index == candidate_field_index) {
                debugln("    is a candidate (",
                        candidate_index,
                        ") for elision");
                insert_unique(argument_order, struct_type, candidate_index);
                break;
              }
            }
            ++candidate_index;
          }
        }
      }
    }

    // For each function that has a used type, we need to change its function
    // type to include the replaced assoc as arguments.
    map<llvm::Function *, map<StructType *, map<size_t, llvm::Value *>>>
        function_to_elision_values = {};
    map<llvm::Function *, llvm::Function *> function_clones = {};
    for (auto const &[function, elision_parameters] :
         function_to_argument_order) {
      // Skip main, it is the entry function.
      if (function->getName() == "main") {
        continue;
      }

      if (function == nullptr || function->empty()) {
        continue;
      }

      // Get information from the old function type.
      auto &old_function_type =
          MEMOIR_SANITIZE(function->getFunctionType(),
                          "Couldn't determine the function type");
      auto *return_type = old_function_type.getReturnType();
      auto param_types = old_function_type.params().vec();
      auto is_var_arg = old_function_type.isVarArg();

      // Get the memoir Collection* type.
      auto *collection_ptr_type =
          FunctionNames::get_memoir_function(M, MemOIR_Func::SIZE)
              ->getFunctionType()
              ->getParamType(0);
      MEMOIR_NULL_CHECK(collection_ptr_type, "Collection* type is NULL!");

      // Add the new argument type.
      auto new_arg_index = param_types.size();
      for (auto elision_param : elision_parameters) {
        param_types.push_back(collection_ptr_type);
      }

      // Construct the new function type.
      auto *new_function_type =
          FunctionType::get(return_type, param_types, is_var_arg);
      MEMOIR_NULL_CHECK(new_function_type,
                        "Could not get the new function type");

      // Create an empty function to clone into.
      auto *new_function = Function::Create(new_function_type,
                                            function->getLinkage(),
                                            function->getName(),
                                            M);
      MEMOIR_NULL_CHECK(new_function, "Could not create new function!");

      debugln("Cloning ", function->getName());

      // Version the function.
      ValueToValueMapTy vmap;
      for (auto &old_argument : function->args()) {
        auto *new_argument =
            new_function->arg_begin() + old_argument.getArgNo();
        vmap.insert(std::make_pair(&old_argument, new_argument));
      }

      llvm::SmallVector<llvm::ReturnInst *, 8> returns;
      llvm::CloneFunctionInto(new_function, function, vmap, false, returns);

      // Collect the elision arguments.
      auto arg_idx = new_arg_index;
      for (auto const &[elision_type, elision_group] : elision_parameters) {
        function_to_elision_values[new_function][elision_type][elision_group] =
            (new_function->arg_begin() + arg_idx++);
      }

      // Record the clone.
      function_clones[function] = new_function;
    }

    // Allocate the associative arrays for each elision candidate in the main
    // function.
    auto &entry_bb = entry_function.getEntryBlock();
    auto *entry_insertion_point = entry_bb.getFirstNonPHI();
    MemOIRBuilder builder(entry_insertion_point, /* InsertAfter= */ false);
    for (auto const &[struct_type, candidates] : fields_to_elide) {
      auto &struct_ref_type = Type::get_ref_type(*struct_type);
      auto *struct_ref_type_inst = builder.CreateTypeInst(struct_ref_type);
      auto candidate_idx = 0;
      for (auto candidate : candidates) {
        if (candidate.size() == 1) {
          // Create the Value type.
          auto candidate_field_index = *(candidate.begin());
          auto &candidate_field_type =
              struct_type->getFieldType(candidate_field_index);
          auto *candidate_type_inst =
              builder.CreateTypeInst(candidate_field_type);

          // Allocate the associative arrays.
          auto *allocation =
              builder.CreateAssocArrayAllocInst(struct_ref_type,
                                                candidate_field_type);

          function_to_elision_values[&entry_function][struct_type]
                                    [candidate_idx] =
                                        &allocation->getCallInst();
        } else {
          MEMOIR_UNREACHABLE("Field elision groups are not supported!");
        }

        ++candidate_idx;
      }
    }

    // Iterate over each function, replacing calls with their clones and adding
    // the relevant arguments.
    for (auto &F : M) {
      // Skip functions that have been cloned.
      if (function_clones.find(&F) != function_clones.end()) {
        continue;
      }

      // Get the elision arguments for this function.
      auto &argument_order = function_to_argument_order[&F];
      auto &elision_values = function_to_elision_values[&F];

      // Collect the calls and accesses.
      set<llvm::CallBase *> calls = {};
      set<AccessInst *> accesses = {};
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *memoir_inst = MemOIRInst::get(I)) {
            auto *access_inst = dyn_cast<AccessInst>(memoir_inst);
            if (access_inst == nullptr) {
              continue;
            }

            if (isa<StructReadInst>(access_inst)
                || isa<StructWriteInst>(access_inst)
                || isa<StructGetInst>(access_inst)) {
              accesses.insert(access_inst);
            }
          } else if (auto *call = dyn_cast<llvm::CallBase>(&I)) {
            calls.insert(call);
          }
        }
      }

      // Update the calls.
      for (auto *call : calls) {
        // Get the called function.
        auto *called_function = call->getCalledFunction();

        // Skip external functions.
        if (called_function == nullptr) {
          continue;
        }

        // Get the clone of the called function.
        auto found_clone = function_clones.find(called_function);
        if (found_clone == function_clones.end()) {
          // If the called function was not cloned, skip it.
          continue;
        }
        auto *cloned_function = found_clone->second;

        debugln("  clone= ", cloned_function->getName());

        // Get the order of arguments that this function needs.
        auto &called_argument_order =
            function_to_argument_order[called_function];

        // Update the list of arguments.
        // TODO: fix this so it works with var args, this will currently be
        // unaligned!
        MEMOIR_ASSERT(!cloned_function->getFunctionType()->isVarArg(),
                      "Cloning function with var args is unsupported!");
        vector<llvm::Value *> new_arguments(call->arg_begin(), call->arg_end());
        for (auto const &[struct_type, elision_group] : called_argument_order) {
          auto *elision_value = elision_values[struct_type][elision_group];
          new_arguments.push_back(elision_value);
          println("new arg: ", *elision_value);
        }

        // Rebuild the call.
        llvm::CallBase *new_call = nullptr;
        if (auto *invoke_inst = dyn_cast<llvm::InvokeInst>(call)) {
          new_call =
              InvokeInst::Create(cloned_function,
                                 invoke_inst->getNormalDest(),
                                 invoke_inst->getUnwindDest(),
                                 llvm::ArrayRef<llvm::Value *>(new_arguments),
                                 "",
                                 call);
        } else if (auto *call_inst = dyn_cast<llvm::CallInst>(call)) {
          new_call =
              CallInst::Create(cloned_function,
                               llvm::ArrayRef<llvm::Value *>(new_arguments),
                               "",
                               call);
          cast<llvm::CallInst>(new_call)->setTailCallKind(
              call_inst->getTailCallKind());
        }

        // Inherit from the original call.
        new_call->setCallingConv(call->getCallingConv());
        new_call->setAttributes(call->getAttributes());
        new_call->copyMetadata(*call,
                               { LLVMContext::MD_prof, LLVMContext::MD_dbg });

        // Replace the old call with the new one.
        call->replaceAllUsesWith(new_call);

        // Take the old name.
        new_call->takeName(call);

        // Erase the old call.
        call->eraseFromParent();
      }

      // Update the accesses.
      for (auto *access : accesses) {
        llvm::CallInst *access_as_call;
        unsigned field_index;
        llvm::Use *field_index_as_use;
        if (auto *struct_read = dyn_cast<StructReadInst>(access)) {
          access_as_call = &struct_read->getCallInst();
          field_index = struct_read->getFieldIndex();
          field_index_as_use = &struct_read->getFieldIndexOperandAsUse();
        } else if (auto *struct_write = dyn_cast<StructWriteInst>(access)) {
          access_as_call = &struct_write->getCallInst();
          field_index = struct_write->getFieldIndex();
          field_index_as_use = &struct_write->getFieldIndexOperandAsUse();
        } else if (auto *struct_get = dyn_cast<StructWriteInst>(access)) {
          access_as_call = &struct_get->getCallInst();
          field_index = struct_get->getFieldIndex();
          field_index_as_use = &struct_get->getFieldIndexOperandAsUse();
        } else {
          continue;
        }

        // Get the struct accessed.
        auto &object_use = access->getObjectOperandAsUse();
        auto *object_value = object_use.get();

        // Get the struct type.
        auto *accessed_type = TypeAnalysis::analyze(*object_value);
        auto *accessed_struct_type = cast<StructType>(accessed_type);

        // Check if this field index is elided.
        size_t elision_group_index = (size_t)-1;
        bool elided = false;
        for (auto const &[struct_type, elision_groups] : fields_to_elide) {
          if (struct_type == accessed_struct_type) {
            elision_group_index = 0;
            for (auto elision_group : elision_groups) {
              for (auto elision_field_index : elision_group) {
                if (elision_field_index == field_index) {
                  elided = true;
                  break;
                }
              }
              if (elided) {
                break;
              }
              ++elision_group_index;
            }
            if (elided) {
              break;
            }
          }
        }
        if (!elided) {
          continue;
        }

        // Get the replacement function.
        auto &original_function =
            MEMOIR_SANITIZE(access_as_call->getCalledFunction(),
                            "Could not find struct function");
        auto &replacement_function =
            MEMOIR_SANITIZE(get_assoc_function(original_function),
                            "Could not find assoc function");

        // Get the elision replacement value.
        auto *replacement_collection =
            elision_values[accessed_struct_type][elision_group_index];

        // Replace the called function.
        access_as_call->setCalledFunction(&replacement_function);

        // Replace the index with the struct being accessed.
        field_index_as_use->set(object_use.get());

        // Replace the struct to the elided collection.
        object_use.set(replacement_collection);
      }
    }

    // Cleanup instructions.
    for (auto *inst : instructions_to_delete) {
      inst->eraseFromParent();
    }

    // Cleanup the originals.
    for (auto [old_function, new_function] : function_clones) {
      new_function->takeName(old_function);
      old_function->eraseFromParent();
    }

    return transformed;
  }
};
} // namespace llvm::memoir

#endif
