#include "llvm/ADT/ArrayRef.h"

#include "ObjectInfo.hpp"

namespace memoir {

struct Heuristic {
  int benefit, cost;

  Heuristic() : benefit(0), cost(0) {}
};

/**
 * Compute the benefit of the given candidate.
 */
Heuristic benefit(llvm::ArrayRef<const ObjectInfo *> candidate);

} // namespace memoir
