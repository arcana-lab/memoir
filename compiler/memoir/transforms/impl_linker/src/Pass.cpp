#include <iostream>
#include <string>

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Analysis/DominanceFrontier.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

// MemOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/support/Timer.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

#include "memoir/lowering/ImplLinker.hpp"
#include "memoir/lowering/TypeLayout.hpp"

using namespace llvm::memoir;

/*
 * This pass collects all collection implementations that will be needed for SSA
 * destruction and instantiates them.
 *
 * Author(s): Tommy McMichen
 * Created: February 19, 2024
 */

namespace llvm::memoir {

llvm::cl::opt<std::string> impl_file_output(
    "impl-out-file",
    llvm::cl::desc("Specify output filename for ImplLinker."),
    llvm::cl::value_desc("filename"));

llvm::PreservedAnalyses ImplLinkerPass::run(llvm::Module &M,
                                            llvm::ModuleAnalysisManager &MAM) {

  // Get the TypeConverter.
  TypeConverter TC(M.getContext());

  // Get the ImplLinker.
  ImplLinker IL(M);

  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    // Collect all of the collection allocations in the program.
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *seq_alloc = into<SequenceAllocInst>(&I)) {

          // Get the implementation name for this allocation.
          auto impl_name = ImplLinker::get_implementation_name(
              I,
              seq_alloc->getCollectionType());

          // Get the type layout for the element type.
          auto &element_layout = TC.convert(seq_alloc->getElementType());

          // Implement the sequence.
          IL.implement_seq(impl_name, element_layout);

        } else if (auto *assoc_alloc = into<AssocAllocInst>(&I)) {

          // Get the implementation name for this allocation.
          auto impl_name = ImplLinker::get_implementation_name(
              I,
              assoc_alloc->getCollectionType());

          // Get the type layout for the key type.
          auto &key_layout = TC.convert(assoc_alloc->getKeyType());

          // Get the type layout for the value type.
          auto &value_layout = TC.convert(assoc_alloc->getValueType());

          // Implement the assoc.
          IL.implement_assoc(impl_name, key_layout, value_layout);
        } else if (auto *define_type = into<DefineStructTypeInst>(&I)) {
          // Get the struct type.
          auto &struct_type = define_type->getType();

          // Get the type layout for the struct.
          auto &struct_layout = TC.convert(struct_type);

          // Implement the struct.
          IL.implement_type(struct_layout);
        }

        // Also handle instructions that don't have selections.
        else {
          // Check that the instruction doesnt already have a selection
          // metadata.
          if (Metadata::get<SelectionMetadata>(I)) {
            continue;
          }

          // Only handle memoir instructions.
          auto *inst = into<MemOIRInst>(&I);
          if (not inst) {
            continue;
          }

#define SET_IMPL "stl_unordered_set"
#define SEQ_IMPL "stl_vector"
#define ASSOC_IMPL "stl_unordered_map"

          // Get the type of the collection being operated on.
          Type *type = nullptr;
          if (auto *access = dyn_cast<AccessInst>(inst)) {
            type = type_of(access->getObjectOperand());
          } else if (auto *insert = dyn_cast<InsertInst>(inst)) {
            type = type_of(insert->getBaseCollection());
          } else if (auto *remove = dyn_cast<RemoveInst>(inst)) {
            type = type_of(remove->getBaseCollection());
          } else if (auto *fold = dyn_cast<FoldInst>(inst)) {
            type = type_of(fold->getCollection());
          } else if (auto *copy = dyn_cast<CopyInst>(inst)) {
            type = type_of(copy->getCopiedCollection());
          } else if (auto *swap = dyn_cast<SwapInst>(inst)) {
            type = type_of(swap->getFromCollection());
          } else if (auto *size = dyn_cast<SizeInst>(inst)) {
            type = type_of(size->getCollection());
          } else if (auto *keys = dyn_cast<AssocKeysInst>(inst)) {
            type = type_of(keys->getCollection());
          }

          // If we couldn't determine a type, skip it.
          if (not type) {
            continue;
          }

          // Link the default implementation.
          if (auto *seq_type = dyn_cast<SequenceType>(type)) {
            // Get the type layout for the element type.
            auto &element_layout = TC.convert(seq_type->getElementType());

            // Implement the sequence.
            IL.implement_seq(SEQ_IMPL, element_layout);

          } else if (auto *assoc_type = dyn_cast<AssocArrayType>(type)) {
            // Get the type layout for the key and value types.
            auto &key_layout = TC.convert(assoc_type->getKeyType());
            auto &val_layout = TC.convert(assoc_type->getValueType());

            if (isa<VoidType>(assoc_type)) {
              IL.implement_assoc(SET_IMPL, key_layout, val_layout);
            } else {
              IL.implement_assoc(ASSOC_IMPL, key_layout, val_layout);
            }
          }
        }
      }
    }
  }

  // Emit the implementation code.
  if (impl_file_output.getNumOccurrences()) {
    std::error_code error;
    llvm::raw_fd_ostream os(impl_file_output, error);
    if (error) {
      MEMOIR_UNREACHABLE("ImplLinker: Could not open output file!");
    }
    IL.emit(os);
  } else {
    IL.emit();
  }

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
