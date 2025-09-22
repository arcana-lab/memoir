#include "memoir/passes/Passes.hpp"
#include "memoir/support/Assert.hpp"

#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Transforms/IPO/Internalize.h"

namespace llvm::memoir {

namespace detail {

void prepare(llvm::Module &M,
             llvm::GlobalValue::LinkageTypes linkage =
                 llvm::GlobalValue::WeakAnyLinkage) {

  llvm::StripDebugInfo(M);

  for (auto &A : M.aliases()) {
    if (!A.isDeclaration()) {
      A.setLinkage(linkage);
    }
  }

  for (auto &F : M) {
    if (!F.isDeclaration()) {
      F.setLinkage(linkage);
    }

    F.setLinkage(linkage);
  }

  return;
}

} // namespace detail

llvm::PreservedAnalyses LinkDeclarationsPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {

#ifndef MEMOIR_INSTALL_PREFIX
#  warning "MEMOIR_INSTALL_PREFIX is unset!"
#endif

  auto buf =
      llvm::MemoryBuffer::getFile(MEMOIR_INSTALL_PREFIX "/lib/memoir.decl.bc");
  auto other = llvm::parseBitcodeFile(*buf.get(), M.getContext());
  auto other_module = std::move(other.get());

  if (other_module->materializeMetadata()) {
    MEMOIR_UNREACHABLE("Failed to link MEMOIR declarations!");
  }

  detail::prepare(*other_module);

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

  // for (auto &F : *other_module) {
  // M.getOrInsertFunction(F.getName(), F.getFunctionType(), F.getAttributes());
  // }

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
