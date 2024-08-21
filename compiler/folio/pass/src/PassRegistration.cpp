// LLVM
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"

// Folio
#include "folio/pass/Pass.hpp"

#include "folio/analysis/ConstraintInference.hpp"
#include "folio/analysis/ContentAnalysis.hpp"

using namespace folio;

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
           "folio",
           LLVM_VERSION_STRING,
           [](llvm::PassBuilder &PB) {
             // Register transforation passes.
             PB.registerPipelineParsingCallback(
                 [](llvm::StringRef name,
                    llvm::ModulePassManager &MPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                   if (name == "folio") {
                     MPM.addPass(folio::FolioPass());
                     return true;
                   }

                   return false;
                 });

             // Register module analyses.
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::ModuleAnalysisManager &MAM) {
                   MAM.registerPass(
                       [&] { return folio::ConstraintInference(); });
                   MAM.registerPass([&] { return folio::ContentAnalysis(); });
                 });

             // Register function analyses.
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::FunctionAnalysisManager &FAM) {
                   // None.
                 });
           } };
}
