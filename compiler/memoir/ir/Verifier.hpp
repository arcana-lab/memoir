#ifndef MEMOIR_IR_VERIFIER_H
#define MEMOIR_IR_VERIFIER_H

namespace llvm::memoir {

class Verifier {
public:
  /**
   * Verifies MEMOIR assumptions for an LLVM function.
   *
   * @param F the LLVM function to verify
   * @param true if the function is broken, false otherwise
   */
  static bool verify(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);

  /**
   * Verifies MEMOIR assumptions for an LLVM module.
   *
   * @param M the LLVM module to verify
   * @param true if the function is broken, false otherwise
   */
  static bool verify(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);

protected:
};

} // namespace llvm::memoir

#endif // MEMOIR_IR_VERIFIER_H
