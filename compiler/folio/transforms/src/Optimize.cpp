#include "folio/transforms/ProxyInsertion.hpp"

using namespace llvm::memoir;

namespace folio {

void ProxyInsertion::optimize() {
  // Optimize the uses in each candidate.
  for (auto &candidate : this->candidates) {
    infoln("OPTIMIZING ", candidate);
    candidate.optimize(this->get_dominator_tree, this->get_bounds_checks);
  }
}

} // namespace folio
