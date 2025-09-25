#ifndef FOLIO_TRANSFORMS_WEAKENUSES_H
#define FOLIO_TRANSFORMS_WEAKENUSES_H

#include "llvm/IR/User.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/support/DataTypes.hpp"

#include "Candidate.hpp"

namespace memoir {

void weaken_uses(
    Set<llvm::Use *> &to_addkey,
    Set<llvm::Use *> &to_weaken,
    Candidate &candidate,
    std::function<BoundsCheckResult &(llvm::Function &)> get_bound_checks);

}

#endif // FOLIO_TRANSFORMS_WEAKENUSES_H
