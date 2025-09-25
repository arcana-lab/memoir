// LLVM
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

// MEMOIR
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Object.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/lowering/Implementation.hpp"
#include "memoir/passes/Passes.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"

namespace memoir {

/*
 * This pass generates multiple bitcode files for all selection permutations.
 *
 * Author(s): Tommy McMichen
 * Created: April 21, 2025
 */

static llvm::cl::opt<std::string> sweep_file_path(
    "memoir-sweep-path",
    llvm::cl::desc("Path where sweep bitcodes and logs should be placed"));

/**
 * Gathers selectable objects in the allocation and populates the map with all
 * candidate selections.
 */
static void gather_candidates(
    OrderedMap<Object, Vector<const Implementation *>> &candidates,
    AllocInst &alloc,
    llvm::ArrayRef<unsigned> offsets,
    Type &type) {

  Vector<unsigned> nested_offsets(offsets);

  if (auto *collection_type = dyn_cast<CollectionType>(&type)) {

    // Populate the list with all possible candidates.
    Object info(alloc.asValue(), offsets);
    Implementation::candidates(*collection_type, candidates[info]);

    // Recurse.
    nested_offsets.push_back(unsigned(-1));
    gather_candidates(candidates,
                      alloc,
                      nested_offsets,
                      collection_type->getElementType());
    nested_offsets.pop_back();

  } else if (auto *tuple_type = dyn_cast<TupleType>(&type)) {

    // Iterate over each field of the tuple.
    for (unsigned field = 0; field < tuple_type->getNumFields(); ++field) {

      // Recurse on the field.
      nested_offsets.push_back(field);
      gather_candidates(candidates,
                        alloc,
                        nested_offsets,
                        tuple_type->getFieldType(field));
      nested_offsets.pop_back();
    }
  }

  return;
}

static void gather_candidates(
    OrderedMap<Object, Vector<const Implementation *>> &candidates,
    AllocInst &alloc) {
  gather_candidates(candidates, alloc, {}, alloc.getType());
}

static void apply_selections(
    OrderedMap<const Object *, const Implementation *> &selections) {

  // Flatten all type mutations for each allocation.
  Map<AllocInst *, Type *> mutated_type = {};
  for (const auto &[info, impl] : selections) {
    auto &alloc = MEMOIR_SANITIZE(into<AllocInst>(info->value()),
                                  "Object value is not an allocation");

    // If we haven't handled this alloc yet, get its type.
    if (not mutated_type.count(&alloc)) {
      mutated_type[&alloc] = &alloc.getType();
    }

    // Set the selection.
    mutated_type[&alloc] = &mutate_selection(*mutated_type[&alloc],
                                             info->offsets(),
                                             impl->get_name());
  }

  // Mutate the type of each allocation.
  for (const auto &[alloc, type] : mutated_type) {
    mutate_type(*alloc, *type);
  }
}

static bool next_permutation(Vector<size_t> &selection,
                             const Vector<size_t> &max) {
  for (size_t i = 0; i < selection.size(); ++i) {
    if (++selection[i] < max[i]) {
      return true;
    }

    if (i + 1 == selection.size()) {
      return false;
    }

    selection[i] = 0;
  }

  return true;
}

static void emit(llvm::Module &M) {
  // Get a fresh filename.
  std::string filename = sweep_file_path + "XXXXXX.bc";
  if (not mkstemps(filename.data(), 3)) {
    MEMOIR_UNREACHABLE("Could not create a temporary file in ",
                       sweep_file_path);
  }

  // Create the output file stream.
  std::error_code error;
  llvm::raw_fd_ostream os(filename,
                          error,
                          llvm::sys::fs::CreationDisposition::CD_CreateAlways);
  if (error) {
    MEMOIR_UNREACHABLE("Could not open ", filename);
  }
  // Write the bitcode to the file.
  llvm::WriteBitcodeToFile(M, os);
}

llvm::PreservedAnalyses SweepSelectionsPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {

  bool modified = false;

  // Register the implementations we want to sweep.
  Implementation::define(
      { Implementation( // std::vector<T>
            "stl_vector",
            SequenceType::get(TypeVariable::get())),

        Implementation( // std::unordered_map<T, U>
            "stl_unordered_map",
            AssocType::get(TypeVariable::get(), TypeVariable::get())),

        Implementation( // std::unordered_set<T>
            "stl_unordered_set",
            AssocType::get(TypeVariable::get(), VoidType::get()))

#ifdef BOOST_INCLUDE_DIR
            ,
        Implementation( // boost::flat_set<T>
            "boost_flat_set",
            AssocType::get(TypeVariable::get(), VoidType::get()),
            /* selectable? */ false),
        Implementation( // boost::flat_map<T>
            "boost_flat_map",
            AssocType::get(TypeVariable::get(), TypeVariable::get()),
            /* selectable? */ false)
#endif

#if 0
            ,
        Implementation( // absl::flat_hash_set<T>
            "abseil_flat_hash_set",
            AssocType::get(TypeVariable::get(), VoidType::get())),
        Implementation( // boost::flat_hash_map<T>
            "abseil_flat_hash_map",
            AssocType::get(TypeVariable::get(), TypeVariable::get()))
#endif

#if 1
            ,
        Implementation("sparse_bitset",
                       AssocType::get(TypeVariable::get(), VoidType::get()),
                       /* selectable? */ false),
        Implementation("sparse_bitmap",
                       AssocType::get(TypeVariable::get(), TypeVariable::get()),
                       /* selectable? */ false),
        Implementation("twined_bitmap",
                       AssocType::get(TypeVariable::get(), TypeVariable::get()),
                       /* selectable? */ false)
#endif

      });

  // Collect all of the possible candidates for each allocation.
  OrderedMap<Object, Vector<const Implementation *>> candidates = {};
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *alloc = into<AllocInst>(I)) {
          gather_candidates(candidates, *alloc);
        }
      }
    }
  }

  for (const auto &[info, impls] : candidates) {
    println(info);
    for (const auto *impl : impls) {
      println("  ", impl->get_name());
    }
    println();
  }

  // Emit bitcodes for all selection permutations.
  Vector<size_t> max_selection(candidates.size(), 0);
  size_t i = 0;
  for (const auto &[info, impls] : candidates) {
    max_selection.at(i++) = impls.size();
  }

  Vector<size_t> selection_indices(candidates.size(), 0);
  OrderedMap<const Object *, const Implementation *> implementations = {};
  do {
    // Gather the selections.
    size_t i = 0;
    for (const auto &[info, impls] : candidates) {
      implementations[&info] = impls.at(selection_indices.at(i++));
    }

    // Transform the program.
    apply_selections(implementations);

    // Emit the bitcode.
    emit(M);

    // Write a log of the selections.
    // write_log(implementations);

  } while (next_permutation(selection_indices, max_selection));

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

} // namespace memoir
