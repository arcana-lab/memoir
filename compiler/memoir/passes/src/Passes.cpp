// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/analysis/LiveRangeAnalysis.hpp"
#include "memoir/analysis/Liveness.hpp"
#include "memoir/analysis/RangeAnalysis.hpp"

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
             // Register transforation passes.
             PB.registerPipelineParsingCallback(
                 [](llvm::StringRef name,
                    llvm::ModulePassManager &MPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {

#define PASS(SCOPE, CLASS, NAME)                                               \
  if (name == NAME) {                                                          \
    MPM.addPass(CLASS());                                                      \
    return true;                                                               \
  }
#include "memoir/passes/Passes.def"
                   return false;
                 });

             // Register module analyses.
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::ModuleAnalysisManager &MAM) {
#define MODULE_ANALYSIS(CLASS, RESULT)                                         \
  MAM.registerPass([&] { return CLASS(); });
#include "memoir/passes/Passes.def"
                   // MAM.registerPass([&] { return
                   // llvm::memoir::RangeAnalysis(); }); MAM.registerPass([&] {
                   // return llvm::memoir::LiveRangeAnalysis(); });
                 });

             // Register function analyses.
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::FunctionAnalysisManager &FAM) {
#define FUNCTION_ANALYSIS(CLASS, RESULT)                                       \
  FAM.registerPass([&] { return CLASS(); });
#include "memoir/passes/Passes.def"
                   // FAM.registerPass([&] { return
                   // llvm::memoir::LivenessAnalysis();
                   // });
                 });
           } };
}
