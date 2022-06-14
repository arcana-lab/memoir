#include "ObjectAbstraction.hpp"

namespace object_abstraction {

  bool ObjectAbstractionPass::doInitialization (Module &M) { 
    return false;
  }

  bool ObjectAbstractionPass::runOnModule (Module &M) {
    auto &noelle = getAnalysis<Noelle>();

    this->objectAbstraction = new object_abstraction::ObjectAbstraction(M, &noelle);
    this->objectAbstraction->contructObjectAbstractions();

    return false;
  }

  void ObjectAbstractionPass::getAnalysisUsage (AnalysisUsage &AU) const {
    AU.addRequired<Noelle>();
  }

  // Next there is code to register your pass to "opt"
  char ObjectAbstractionPass::ID = 0;
  static RegisterPass<ObjectAbstractionPass> X("ObjectAbstraction", "Analysis pass to build object representations at LLVM-IR level");

  // Next there is code to register your pass to "clang"
  static ObjectAbstractionPass * _PassMaker = NULL;
  static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
      [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
          if(!_PassMaker){ PM.add(_PassMaker = new ObjectAbstractionPass());}}); // ** for -Ox
  static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
      [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
          if(!_PassMaker){ PM.add(_PassMaker = new ObjectAbstractionPass()); }}); // ** for -O0

} // namespace object_abstraction