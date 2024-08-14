#ifndef FOLIO_PASS_H
#define FOLIO_PASS_H

#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

namespace folio {

class FolioPass : public llvm::PassInfoMixin<FolioPass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};

} // namespace folio

#endif // FOLIO_PASS_H
