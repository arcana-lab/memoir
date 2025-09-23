#ifndef FOLIO_TRANSFORMS_WEAKENUSES_H
#define FOLIO_TRANSFORMS_WEAKENUSES_H

#include "llvm/IR/User.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/Candidate.hpp"

namespace folio {

void weaken_uses(memoir::Set<llvm::Use *> &to_addkey,
                 memoir::Set<llvm::Use *> &to_weaken,
                 Candidate &candidate,
                 std::function<memoir::BoundsCheckResult &(llvm::Function &)>
                     get_bound_checks);

}

#endif // FOLIO_TRANSFORMS_WEAKENUSES_H
