#include <algorithm>
#include <numeric>

#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"

#include "memoir/lowering/Implementation.hpp"
#include "memoir/transforms/utilities/ReifyTempArgs.hpp"

#include "folio/transforms/CoalesceUses.hpp"
#include "folio/transforms/ProxyInsertion.hpp"
#include "folio/transforms/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

// ================

static llvm::cl::opt<std::string> proxy_set_impl(
    "proxy-set-impl",
    llvm::cl::desc("Set the implementation for proxied sets"),
    llvm::cl::init("bitset"));

static llvm::cl::opt<std::string> proxy_nested_set_impl(
    "proxy-nested-set-impl",
    llvm::cl::desc("Set the implementation for proxied sets that are nested"),
    llvm::cl::init(proxy_set_impl));

static llvm::cl::opt<std::string> proxy_map_impl(
    "proxy-map-impl",
    llvm::cl::desc("Set the implementation for proxied map"),
    llvm::cl::init("bitmap"));

Option<std::string> ProxyInsertion::get_enumerated_impl(Type &type,
                                                        bool is_nested) {
  if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
    auto selection = collection_type->get_selection();

    if (selection) {
      return selection;
    }

    if (auto *assoc_type = dyn_cast<AssocType>(collection_type)) {
      if (isa<VoidType>(&assoc_type->getValueType())) {
        return is_nested ? proxy_nested_set_impl : proxy_set_impl;
      } else {
        return proxy_map_impl;
      }
    }

    return selection;
  }

  return {};
}

ProxyInsertion::ProxyInsertion(llvm::Module &M,
                               GetDominatorTree get_dominator_tree,
                               GetBoundsChecks get_bounds_checks)
  : M(M),
    get_dominator_tree(get_dominator_tree),
    get_bounds_checks(get_bounds_checks) {

  // Register the bit{map,set} implementations.
  Implementation::define({
      Implementation(proxy_set_impl,
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation(proxy_nested_set_impl,
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation(proxy_map_impl,
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

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }

  MemOIRInst::invalidate();

  reify_tempargs(M);

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
}

} // namespace folio
