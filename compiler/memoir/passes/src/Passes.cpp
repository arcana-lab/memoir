// LLVM
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

using namespace llvm::memoir;

// Helper function to adapt function pass to a module pass.
template <typename T>
static auto adapt(T &&fp) {
  return llvm::createModuleToFunctionPassAdaptor(std::move(fp));
}

// Macro to register simple passes with the pass manager
#define REGISTER(PASS_NAME, PASS_TYPE, PASS_ARGS...)                           \
  if (name == PASS_NAME) {                                                     \
    MPM.addPass(PASS_TYPE(PASS_ARGS));                                         \
    return true;                                                               \
  }

// Register the passes and pipelines with the new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return { LLVM_PLUGIN_API_VERSION,
           "memoir",
           LLVM_VERSION_STRING,
           [](llvm::PassBuilder &PB) {
             PB.registerPipelineParsingCallback(
                 [](llvm::StringRef name,
                    llvm::ModulePassManager &MPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                   REGISTER("memoir-ssa-construction",
                            llvm::memoir::SSAConstructionPass);
                   REGISTER("memoir-ssa-destruction",
                            llvm::memoir::SSADestructionPass);
                   REGISTER("memoir-impl-linker", llvm::memoir::ImplLinkerPass);
                   REGISTER("memoir-norm", llvm::memoir::NormalizationPass);

                   return false;
                 });
           } };
}
