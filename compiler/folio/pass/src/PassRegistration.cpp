// LLVM
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

// Folio
#include "folio/pass/Pass.hpp"

// #include "folio/analysis/ConstraintInference.hpp"
// #include "folio/analysis/ContentAnalysis.hpp"

// #include "folio/opportunities/Analysis.hpp"

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

void folio_selection(llvm::ModulePassManager &MPM) {
  MPM.addPass(folio::FolioPass());
  MPM.addPass(llvm::memoir::TempArgReificationPass());

  return;
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
                   if (name == "folio-selection") {
                     folio_selection(MPM);
                     return true;
                   }

                   if (name == "folio") {
                     llvm::memoir::raise_memoir(MPM);
                     folio_selection(MPM);
                     llvm::memoir::lower_memoir(MPM);
                     return true;
                   }

                   return false;
                 });

             // Register module analyses.
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::ModuleAnalysisManager &MAM) {
                   // MAM.registerPass([&] { return
                   // folio::ConstraintInference(); }); MAM.registerPass([&] {
                   // return folio::ContentAnalysis(); }); MAM.registerPass([&]
                   // { return folio::OpportunityAnalysis(); });
                   // #define OPPORTUNITY(CLASS) \
//   MAM.registerPass([&] { return folio::CLASS##Analysis();
                   //   });
                   // #include "folio/opportunities/Opportunities.def"
                 });

             // Register function analyses.
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::FunctionAnalysisManager &FAM) {
                   // None.
                 });
           } };
}
