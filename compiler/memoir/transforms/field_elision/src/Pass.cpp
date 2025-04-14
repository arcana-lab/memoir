#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/CommandLine.h"

#include "llvm/Analysis/CallGraph.h"

// MemOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"

// Field Elision
#include "FieldElision.hpp"

using namespace llvm::memoir;

/*
 * This pass performs the field elision optimization.
 *
 * Author(s): Tommy McMichen
 * Created: August 25, 2023
 */

namespace llvm::memoir {

static llvm::cl::List<std::string> FieldsToElide(
    "elide",
    cl::desc("Specify fields to elide as NAME:FIELD#,..."),
    cl::ZeroOrMore);

llvm::PreservedAnalyses FieldElisionPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {
  infoln("========================");
  infoln("BEGIN field elision pass");
  infoln();

  // TODO: when we update the escape analysis to use the complete call graph,
  // also update users of this call graph to do the same..
  auto &CG = MAM.getResult<llvm::CallGraphAnalysis>(M);

  // Parse the input fields to elide.
  FieldElision::FieldsToElideMapTy fields_to_elide = {};

  // Find all user type definitions in the program.
  Map<std::string, DefineTupleTypeInst *> type_definitions = {};
  auto *define_struct_type_func =
      FunctionNames::get_memoir_function(M, MemOIR_Func::DEFINE_STRUCT_TYPE);
  for (auto &func_use : define_struct_type_func->uses()) {
    auto *func_user = func_use.getUser();
    auto *func_user_as_inst = dyn_cast_or_null<llvm::Instruction>(func_user);
    if (auto *define_inst = into<DefineTupleTypeInst>(func_user_as_inst)) {
      auto type_name = define_inst->getName();
      type_definitions[type_name] = define_inst;
    }
  }

  // Parse the list of elision candidates.
  for (auto &elide_str : FieldsToElide) {
    infoln("Elision candidate: ", elide_str);

    // Parse the type name.
    size_t token_index = 0;
    size_t next_index = elide_str.find(':');
    auto type_name = elide_str.substr(token_index, next_index);

    // Find the named type, if it exists.
    auto found_type = type_definitions.find(type_name);
    if (found_type == type_definitions.end()) {
      warnln("Elision candidate struct type ",
             type_name,
             " does not exist in the llvm Module!");
      continue;
    }

    auto *type_definition = found_type->second;
    auto &elide_type = type_definition->getType();
    auto &elide_struct_type =
        MEMOIR_SANITIZE(dyn_cast<TupleType>(&elide_type),
                        "Elided type is not a struct type!");

    memoir::info("elision candidate: ", type_name, ".");
    FieldElision::IndexSetTy field_indices = {};
    for (token_index = next_index + 1; next_index < elide_str.size();
         token_index = next_index + 1) {
      // Find the end of this token.
      next_index = elide_str.find(',', token_index);
      // Extract the token.
      auto index_str = elide_str.substr(token_index, next_index);
      memoir::info(index_str, ",");

      // Insert the field index into the set.
      field_indices.insert(std::stoi(index_str));
    }
    infoln();

    fields_to_elide[&elide_struct_type].push_back(field_indices);
  }

  // Perform field elision on the candidates.
  FieldElision FE(M, CG, fields_to_elide);

  infoln();
  infoln("END field elision pass");
  infoln("========================");

  return FE.transformed ? llvm::PreservedAnalyses::none()
                        : llvm::PreservedAnalyses::all();
}

} // namespace llvm::memoir
