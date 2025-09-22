#include "Normalization.hpp"

#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/utility/FunctionNames.hpp"

#include "llvm/Transforms/Utils/ModuleUtils.h"

namespace llvm::memoir {

void Normalization::transformRuntime() {

  Set<llvm::Function *> functions_to_delete;
  for (auto &F : M) {
    if (FunctionNames::is_memoir_call(F) || FunctionNames::is_mut_call(F)) {
      F.deleteBody();
    } else {
      functions_to_delete.insert(&F);
    }
  }

  for (auto func : functions_to_delete) {
    debugln("Deleting ", func->getName());
    func->removeFromParent();
  }

  return;
}

} // namespace llvm::memoir
