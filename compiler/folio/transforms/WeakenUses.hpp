#ifndef FOLIO_TRANSFORMS_WEAKENUSES_H
#define FOLIO_TRANSFORMS_WEAKENUSES_H

#include "llvm/IR/User.h"

#include "memoir/support/DataTypes.hpp"

#include "folio/transforms/ProxyInsertion.hpp"

namespace folio {

void weaken_uses(llvm::memoir::Set<llvm::Use *> &to_addkey,
                 llvm::memoir::Set<llvm::Use *> &to_weaken,
                 Candidate &candidate,
                 ProxyInsertion::GetBoundsChecks get_bound_checks);

}

#endif // FOLIO_TRANSFORMS_WEAKENUSES_H
