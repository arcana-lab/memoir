#include "llvm/ADT/ArrayRef.h"

#include "folio/ObjectInfo.hpp"

namespace folio {

/**
 * Compute the benefit of the given candidate.
 */
int benefit(llvm::ArrayRef<const ObjectInfo *> candidate);

} // namespace folio
