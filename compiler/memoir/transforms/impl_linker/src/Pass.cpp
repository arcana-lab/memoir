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
#include "llvm/IR/Verifier.h"

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
#include "memoir/support/DataTypes.hpp"
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
static void link_implementation(ImplLinker &IL, CollectionType &type) {

  // Implement each of the required collection types.
  auto *collection_type = &type;
  while (collection_type) {

    // Get the implementation.
    const Implementation *impl = nullptr;

    // Check if there is an explicit selection.
    auto selection = collection_type->get_selection();
    if (selection) {
      impl = Implementation::lookup(selection.value());
    }

    // Get the default implementation if none was selected.
    if (not impl) {
      impl = &ImplLinker::get_default_implementation(*collection_type);
    }

    // Implement the selection.
    // TODO: this needs to change to support impls N-dimensional impls
    if (Type::is_unsized(*collection_type)) {

      // Instantiate the implementation.
      auto &inst = impl->instantiate(*collection_type);

      IL.implement(inst);
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

          // Link the selection.
          detail::link_implementation(IL, *collection_type);

        } else if (auto *tuple_inst = into<TupleTypeInst>(&I)) {
          // Get the struct type.
          auto &tuple_type = cast<TupleType>(tuple_inst->getType());

          // Implement the struct.
          IL.implement(tuple_type);

          // For each collection-typed field, fetch the selection, if it exists.
          for (unsigned field_index = 0;
               field_index < tuple_type.getNumFields();
               ++field_index) {
            auto &field_type = tuple_type.getFieldType(field_index);

            // If the field is a collection, implement it.
            if (auto *collection_type = dyn_cast<CollectionType>(&field_type)) {
              detail::link_implementation(IL, *collection_type);
            }
          }
        }

        // Also handle instructions that don't have selections.
        else {

          // Only handle memoir instructions.
          auto *inst = into<MemOIRInst>(&I);
          if (not inst) {
            continue;
          }

          // Get the type of the collection being operated on.
          Type *type = nullptr;
          if (auto *access = dyn_cast<AccessInst>(inst)) {
            type = type_of(access->getObject());
          }

          // If we couldn't determine a type, skip it.
          auto *collection_type = dyn_cast_or_null<CollectionType>(type);
          if (not collection_type) {
            continue;
          }

          // Link the default implementation.
          auto &default_impl =
              ImplLinker::get_default_implementation(*collection_type);

          // Instantiate the default implementation.
          auto &default_inst = default_impl.instantiate(*collection_type);

          IL.implement(default_inst);
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
                             "-std=c++20",
                             // "-g",
                             "-c",
                             "-emit-llvm",
                             "-Wall",
                             "-I" MEMOIR_INSTALL_PREFIX "/include",
#ifdef BOOST_INCLUDE_DIR
                             "-I" BOOST_INCLUDE_DIR "/include",
#endif
                             "-o",
                             bitcode_filename };
  std::string message = "";
  bool failed = false;
  print("Compiling implementations...");
  llvm::sys::ExecuteAndWait(clang_path,
                            args,
                            std::nullopt,
                            {},
                            0,
                            0,
                            &message,
                            &failed);

  if (failed) {
    println("FAILED!");
    MEMOIR_UNREACHABLE(
        "clang++ failed to compile implementations with message:\n",
        message);
  } else {
    print("\r                                                    \r");
  }

  // Load the new bitcode.
  auto buf = llvm::MemoryBuffer::getFile(bitcode_filename);
  auto other = llvm::parseBitcodeFile(*buf.get(), M.getContext());
  auto other_module = std::move(other.get());

  // Prepare the module for linking.
  const llvm::GlobalValue::LinkageTypes weak =
      llvm::GlobalValue::WeakAnyLinkage;
  const llvm::GlobalValue::LinkageTypes external =
      llvm::GlobalValue::ExternalLinkage;

  for (auto &A : other_module->aliases()) {
    if (A.isDeclaration()) {
      A.setLinkage(external);
    } else {
      A.setLinkage(weak);
    }
  }

  for (auto &G : other_module->globals()) {
    // Skip intrinsics.
    if (G.hasInitializer() and G.hasName()) {
      auto name = G.getName();
      if (name == "llvm.used" or name == "llvm.compiler.used"
          or name == "llvm.global_ctors" or name == "llvm.global_dtors") {
        continue;
      }
    }

    if (G.isDeclaration()) {
      G.setLinkage(external);
    } else {
      G.setLinkage(weak);
    }
  }

  for (auto &F : *other_module) {
    if (F.isDeclaration()) {
      F.setLinkage(external);
    } else {
      F.setLinkage(weak);
    }
  }

  // Verify both modules before linking.
  print("Verifying program module... ");
  if (llvm::verifyModule(M, &llvm::errs())) {
    println("FAILED!");
    MEMOIR_UNREACHABLE("Failed to verify the original module.");
  } else {
    print("\r                                                    \r");
  }

  print("Verifying implementations module... ");
  if (llvm::verifyModule(*other_module, &llvm::errs())) {
    println("FAILED!");
    MEMOIR_UNREACHABLE("Failed to verify the implementations module.");
  } else {
    print("\r                                                    \r");
  }

  // Link the modules.
  print("Linking implementations... ");
  if (llvm::Linker::linkModules(
          M,
          std::move(other_module),
          llvm::Linker::Flags::OverrideFromSrc,
          [](llvm::Module &M, const llvm::StringSet<> &GVS) {
            llvm::internalizeModule(M, [&GVS](const llvm::GlobalValue &GV) {
              return !GV.hasName() || (GVS.count(GV.getName()) == 0);
            });
          })) {
    println("FAILED!");
    MEMOIR_UNREACHABLE("LLVM Linker failed to link in MEMOIR declarations!");
  } else {
    print("\r                                                    \r");
  }

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
