#include <algorithm>
#include <numeric>

#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"

#include "memoir/lowering/Implementation.hpp"
#include "memoir/transform/utilities/ReifyTempArgs.hpp"

#include "DataEnumeration.hpp"
#include "Utilities.hpp"

using namespace memoir;

namespace memoir {

// ================

static llvm::cl::opt<std::string> ade_set_impl(
    "ade-set-impl",
    llvm::cl::desc("Set the implementation for proxied sets"),
    llvm::cl::init("bitset"));

static llvm::cl::opt<std::string> ade_nested_set_impl(
    "ade-nested-set-impl",
    llvm::cl::desc("Set the implementation for proxied sets that are nested"),
    llvm::cl::init(ade_set_impl));

static llvm::cl::opt<std::string> ade_map_impl(
    "ade-map-impl",
    llvm::cl::desc("Set the implementation for proxied map"),
    llvm::cl::init("bitmap"));

Option<std::string> DataEnumeration::get_enumerated_impl(Type &type,
                                                         bool is_nested) {
  if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
    auto selection = collection_type->get_selection();

    if (selection) {
      return selection;
    }

    if (auto *assoc_type = dyn_cast<AssocType>(collection_type)) {
      if (isa<VoidType>(&assoc_type->getValueType())) {
        return is_nested ? ade_nested_set_impl : ade_set_impl;
      } else {
        return ade_map_impl;
      }
    }

    return selection;
  }

  return {};
}

DataEnumeration::DataEnumeration(llvm::Module &module,
                                 GetDominatorTree get_dominator_tree,
                                 GetBoundsChecks get_bounds_checks)
  : module(module),
    get_dominator_tree(get_dominator_tree),
    get_bounds_checks(get_bounds_checks) {

  // Register the bit{map,set} implementations.
  Implementation::define({
      Implementation(ade_set_impl,
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation(ade_nested_set_impl,
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation(ade_map_impl,
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

      Implementation("bitset",
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation("bitmap",
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

      Implementation("sparse_bitset",
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation("sparse_bitmap",
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

      Implementation("twined_bitmap",
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

  });

  // Analyze the objects to find candidates.
  this->analyze();

  // Optimize the uses in each candidate.
  this->optimize();

  // Prepare the program for transformation.
  this->prepare();

  // Transform the program.
  this->transform();

  for (auto &function : this->module) {
    if (not function.empty()) {
      print("VERIFYING ", function.getName());
      if (llvm::verifyFunction(function, &llvm::errs())) {
        println(function);
        MEMOIR_UNREACHABLE("Failed to verify ", function.getName());
      } else {
        print("\r                                                \r");
      }
    }
  }

  MemOIRInst::invalidate();

  reify_tempargs(this->module);

  for (auto &function : this->module) {
    if (not function.empty()) {
      print("VERIFYING ", function.getName());
      if (llvm::verifyFunction(function, &llvm::errs())) {
        println(function);
        MEMOIR_UNREACHABLE("Failed to verify ", function.getName());
      } else {
        print("\r                                                \r");
      }
    }
  }
}

} // namespace memoir
