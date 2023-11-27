#include "NoelleWrapper.hpp"

namespace llvm::memoir {

NoelleWrapper::NoelleWrapper()
  : ModulePass(ID),
    noelle(nullptr),
    dependenceAnalysis("MemOIR Dependence Analysis") {}

Noelle &NoelleWrapper::getNoelle(void) {
  if (!noelle) {
    noelle = &getAnalysis<llvm::noelle::Noelle>();
    noelle->addAnalysis(&dependenceAnalysis);
  }
  return *noelle;
}

bool NoelleWrapper::doInitialization(Module &M) {
  return false;
}

bool NoelleWrapper::runOnModule(Module &M) {
  return false;
}

void NoelleWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<llvm::noelle::Noelle>();
}

// Next there is code to register your pass to "opt"
char NoelleWrapper::ID = 0;
static RegisterPass<NoelleWrapper> X("NoelleWrapper",
                                     "Get a MemOIR-aware Noelle instance");

// Next there is code to register your pass to "clang"
static NoelleWrapper *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
                                        [](const PassManagerBuilder &,
                                           legacy::PassManagerBase &PM) {
                                          if (!_PassMaker) {
                                            PM.add(_PassMaker =
                                                       new NoelleWrapper());
                                          }
                                        }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new NoelleWrapper());
      }
    }); // ** for -O0

} // namespace llvm::memoir
