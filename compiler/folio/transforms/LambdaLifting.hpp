#ifndef FOLIO_LAMBDALIFTING_H
#define FOLIO_LAMBDALIFTING_H

#include "llvm/IR/Module.h"

namespace folio {

class LambdaLifting {
public:
  LambdaLifting(llvm::Module &M);

protected:
  llvm::Module &M;
};

} // namespace folio

#endif // FOLIO_LAMBDALIFTING_H
