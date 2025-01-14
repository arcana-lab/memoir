#include <iostream>
#include <string>

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Analysis/DominanceFrontier.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Transforms/IPO/Internalize.h"

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
    llvm::cl::value_desc("filename"),
    llvm::cl::init("/tmp/XXXXXX.cpp"));

namespace detail {
void link_implementation(ImplLinker &IL,
                         TypeConverter &TC,
                         CollectionType &type,
                         std::optional<SelectionMetadata> selection,
                         unsigned selection_index = 0) {

  // Implement each of the required collection types.
  auto *collection_type = &type;
  while (collection_type) {
    // Get the implementation.
    const Implementation *impl = nullptr;
    if (selection.has_value()) {
      auto selected_name = selection->getImplementation(selection_index++);
      impl = Implementation::lookup(selected_name);
    }

    // Get the default implementation if none was selected.
    if (not impl) {
      impl = &ImplLinker::get_default_implementation(*collection_type);
    }

    // Implement the selection.
    // TODO: this needs to change to support impls N-dimensional impls
    if (auto *seq_type = dyn_cast<SequenceType>(collection_type)) {
      // Implement the sequence.
      auto &element_layout = TC.convert(seq_type->getElementType());

      IL.implement_seq(impl->get_name(), element_layout);

    } else if (auto *assoc_type = dyn_cast<AssocType>(collection_type)) {
      // Implement the assoc.
      auto &key_layout = TC.convert(assoc_type->getKeyType());
      auto &val_layout = TC.convert(assoc_type->getValueType());

      IL.implement_assoc(impl->get_name(), key_layout, val_layout);
    }

    // Get the next collection type to match.
    for (unsigned dim = 0; dim < impl->num_dimensions(); ++dim) {
      MEMOIR_ASSERT(collection_type,
                    "Could not match implementation on a non-collection type.");
      auto &elem_type = collection_type->getElementType();
      collection_type = dyn_cast<CollectionType>(&elem_type);
    }
  }

  return;
}
} // namespace detail

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
        if (auto *alloc = into<AllocInst>(&I)) {

          // Check that this is a collection implementation.
          auto *collection_type = dyn_cast<CollectionType>(&alloc->getType());
          if (not collection_type) {
            continue;
          }

          // Fetch the selection from the instruction metadata, if it exists.
          auto selection = Metadata::get<SelectionMetadata>(I);

          // Link the selection.
          detail::link_implementation(IL, TC, *collection_type, selection);

        } else if (auto *define_type = into<DefineStructTypeInst>(&I)) {
          // Get the struct type.
          auto &struct_type = define_type->getStructType();

          // Get the type layout for the struct.
          auto &struct_layout = TC.convert(struct_type);

          // Implement the struct.
          IL.implement_type(struct_layout);

          // For each collection-typed field, fetch the selection, if it exists.
          for (unsigned field_index = 0;
               field_index < struct_type.getNumFields();
               ++field_index) {
            auto &field_type = struct_type.getFieldType(field_index);

            // If the field is a collection, implement it.
            if (auto *collection_type = dyn_cast<CollectionType>(&field_type)) {

              // Fetch the selection for the given field.
              auto selection =
                  Metadata::get<SelectionMetadata>(struct_type, field_index);

              detail::link_implementation(IL, TC, *collection_type, selection);
            }
          }
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
            type = type_of(access->getObject());
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

            if (isa<VoidType>(assoc_type->getValueType())) {
              IL.implement_assoc(SET_IMPL, key_layout, val_layout);
            } else {
              IL.implement_assoc(ASSOC_IMPL, key_layout, val_layout);
            }
          }
        }
      }
    }
  }

  // If we were not given a file to output to, create a temporary file.
  if (impl_file_output.getNumOccurrences() == 0) {
    if (not mkstemps(impl_file_output.getValue().data(), 4)) {
      MEMOIR_UNREACHABLE("ImplLinker: Could not create a temporary file!");
    }
  }

  // Emit the implementation code.
  std::error_code error;
  llvm::raw_fd_ostream os(impl_file_output,
                          error,
                          llvm::sys::fs::CreationDisposition::CD_CreateAlways);
  if (error) {
    MEMOIR_UNREACHABLE("ImplLinker: Could not open output file!");
  }
  IL.emit(os);

  println("ImplLinker: C++ implementation at ", impl_file_output);

  os.close();

  // Create a temporary bitcode file.
  std::string bitcode_filename = "/tmp/XXXXXX.bc";
  if (not mkstemps(bitcode_filename.data(), 3)) {
    MEMOIR_UNREACHABLE(
        "ImplLinker: Could not create a temporary bitcode file!");
  }

  println("ImplLinker: bitcodes at ", bitcode_filename);

  // Compile the file.
  // TODO: can we get -march=XXX from LLVM and use it here?
  // auto clang_program = llvm::sys::findProgramByName();
  const char *clang_path = LLVM_BIN_DIR "/clang++";
  llvm::StringRef args[] = { clang_path,
                             impl_file_output,
                             "-O3",
                             "-gdwarf-4",
                             "-c",
                             "-emit-llvm",
                             "-I" MEMOIR_INSTALL_PREFIX "/include",
                             "-o",
                             bitcode_filename };
  std::string message = "";
  bool failed = false;
  llvm::sys::ExecuteAndWait(clang_path,
                            args,
                            std::nullopt,
                            {},
                            0,
                            0,
                            &message,
                            &failed);

  if (failed) {
    println("ImplLinker: clang++ failed with:");
    println(message);
    MEMOIR_UNREACHABLE("clang++ failed to compile implementations, see above.");
  }

  // Load the new bitcode.
  auto buf = llvm::MemoryBuffer::getFile(bitcode_filename);
  auto other = llvm::parseBitcodeFile(*buf.get(), M.getContext());
  auto other_module = std::move(other.get());

  // Prepare the module for linking.
  const llvm::GlobalValue::LinkageTypes linkage =
      llvm::GlobalValue::WeakAnyLinkage;

  for (auto &A : other_module->aliases()) {
    if (!A.isDeclaration()) {
      A.setLinkage(linkage);
    }
  }

  for (auto &F : *other_module) {
    if (!F.isDeclaration()) {
      F.setLinkage(linkage);
    }

    F.setLinkage(linkage);
  }

  // Link the modules.
  if (llvm::Linker::linkModules(
          M,
          std::move(other_module),
          llvm::Linker::Flags::OverrideFromSrc,
          [](llvm::Module &M, const llvm::StringSet<> &GVS) {
            llvm::internalizeModule(M, [&GVS](const llvm::GlobalValue &GV) {
              return !GV.hasName() || (GVS.count(GV.getName()) == 0);
            });
          })) {
    MEMOIR_UNREACHABLE("LLVM Linker failed to link in MEMOIR declarations!");
  }

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
