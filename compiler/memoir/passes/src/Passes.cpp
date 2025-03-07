// LLVM
#include "llvm/Analysis/CGSCCPassManager.h"

#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/ArgumentPromotion.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/InstSimplifyPass.h"
#include "llvm/Transforms/Scalar/LoopSimplifyCFG.h"
#include "llvm/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/BreakCriticalEdges.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/analysis/LiveRangeAnalysis.hpp"
#include "memoir/analysis/Liveness.hpp"
#include "memoir/analysis/RangeAnalysis.hpp"

using namespace llvm::memoir;

// Helper function to adapt function pass to a module pass.
template <typename T>
static auto adapt_function(T &&fp) {
  return llvm::createModuleToFunctionPassAdaptor(std::move(fp));
}

// Helper function to adapt CGSCC pass to a module pass.
template <typename T>
static auto adapt_cgscc(T &&cp) {
  return llvm::createModuleToPostOrderCGSCCPassAdaptor(std::move(cp));
}

// Helper function to adapt loop pass to a module pass.
template <typename T>
static auto adapt_loop(T &&lp) {
  return llvm::createModuleToFunctionPassAdaptor(
      llvm::createFunctionToLoopPassAdaptor(std::move(lp)));
}

// Macro to register simple passes with the pass manager
#define REGISTER(PASS_NAME, PASS_TYPE, PASS_ARGS...)                           \
  if (name == PASS_NAME) {                                                     \
    MPM.addPass(PASS_TYPE(PASS_ARGS));                                         \
    return true;                                                               \
  }

namespace llvm::memoir {

void raise_memoir(llvm::ModulePassManager &MPM) {

  // LLVM normalizations.
  MPM.addPass(llvm::AlwaysInlinerPass());
  MPM.addPass(adapt_cgscc(llvm::InlinerPass()));
  MPM.addPass(adapt_cgscc(
      llvm::ArgumentPromotionPass(/* Don't limit max elements */ 0)));
  MPM.addPass(adapt_function(llvm::SROAPass(llvm::SROAOptions::PreserveCFG)));
  MPM.addPass(adapt_function(llvm::PromotePass()));
  MPM.addPass(adapt_function(llvm::InstSimplifyPass()));
  MPM.addPass(adapt_function(llvm::LowerSwitchPass()));
  MPM.addPass(adapt_function(llvm::UnifyFunctionExitNodesPass()));
  MPM.addPass(adapt_function(llvm::BreakCriticalEdgesPass()));
  MPM.addPass(adapt_loop(llvm::LoopSimplifyCFGPass()));
  MPM.addPass(adapt_function(llvm::LCSSAPass()));
  MPM.addPass(adapt_loop(llvm::IndVarSimplifyPass()));
  MPM.addPass(llvm::GlobalDCEPass());

  // Link MEMOIR declarations.
  MPM.addPass(LinkDeclarationsPass());

  // Infer types.
  MPM.addPass(TypeInferencePass());

  // Construct SSA form.
  MPM.addPass(SSAConstructionPass());

  // Infer types.
  MPM.addPass(TypeInferencePass());

  // LLVM normalizations.
  MPM.addPass(adapt_function(llvm::SimplifyCFGPass()));
  MPM.addPass(adapt_loop(llvm::LoopSimplifyCFGPass()));
  MPM.addPass(adapt_function(llvm::LCSSAPass()));

  // Insert live-out metadata.
  MPM.addPass(adapt_function(LiveOutInsertionPass()));

  return;
}

void lower_memoir(llvm::ModulePassManager &MPM) {

  // memoir-impl-linker
  MPM.addPass(ImplLinkerPass());

  // memoir-ssa-destruction
  MPM.addPass(SSADestructionPass());

  return; // TEMPORARY

  // always-inline
  MPM.addPass(llvm::AlwaysInlinerPass());

  // mem2reg
  MPM.addPass(adapt_function(llvm::PromotePass()));

  // memcpyopt
  MPM.addPass(adapt_function(llvm::MemCpyOptPass()));

  // global-dce
  MPM.addPass(llvm::GlobalDCEPass());
}

} // namespace llvm::memoir

// Register the passes and pipelines with the new pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return { LLVM_PLUGIN_API_VERSION,
           "memoir",
           LLVM_VERSION_STRING,
           [](llvm::PassBuilder &PB) {
#if 0
      // Raise MEMOIR at the start of LTO
      PB.registerFullLinkTimeOptimizationEarlyEPCallback(
          [](llvm::ModulePassManager &MPM, llvm::OptimizationLevel level) {
            if (level == llvm::OptimizationLevel::O3) {
              // raise_memoir(MPM);
            }
          });

      // Lower MEMOIR at the end of LTO
      PB.registerFullLinkTimeOptimizationLastEPCallback(
          [](llvm::ModulePassManager &MPM, llvm::OptimizationLevel level) {
            if (level == llvm::OptimizationLevel::O3) {
              // lower_memoir(MPM);
            }
          });
#endif
             // Register module transformation passes.
             PB.registerPipelineParsingCallback(
                 [](llvm::StringRef name,
                    llvm::ModulePassManager &MPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                   // Pass that converts LLVM to MEMOIR
                   if (name == "raise-memoir") {

                     raise_memoir(MPM);

                     return true;
                   }

                   // Pass that converts MEMOIR to LLVM
                   if (name == "lower-memoir") {
                     lower_memoir(MPM);

                     return true;
                   }

                   if (name == "memoir") {

                     raise_memoir(MPM);

                     // TODO: add optimization levels

                     lower_memoir(MPM);

                     return true;
                   }

                   // LowerFold require some addition simplification to be run
                   // after it so it doesn't break other passes in the pipeline.
                   if (name == "memoir-lower-fold") {
                     MPM.addPass(LowerFoldPass());
                     MPM.addPass(adapt_function(llvm::LoopSimplifyPass()));
                     MPM.addPass(adapt_function(llvm::LCSSAPass()));
                     return true;
                   }

#define MODULE_PASS(CLASS, NAME)                                               \
  if (name == NAME) {                                                          \
    MPM.addPass(CLASS());                                                      \
    return true;                                                               \
  }
#include "memoir/passes/Passes.def"

                   return false;
                 });

             // Register module transformation passes.
             PB.registerPipelineParsingCallback(
                 [](llvm::StringRef name,
                    llvm::FunctionPassManager &FPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
#define FUNCTION_PASS(CLASS, NAME)                                             \
  if (name == NAME) {                                                          \
    FPM.addPass(CLASS());                                                      \
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
