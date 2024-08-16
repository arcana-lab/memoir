#include "folio/opportunities/ProxyOpportunity.hpp"

using namespace llvm::memoir;

namespace folio {

// ===========================
// ProxyOpportunity
template <>
std::string Opportunity::formulate<ProxyOpportunity>() {
  std::string formula;

  return formula;
}

std::string ProxyOpportunity::formulate() const {
  std::string formula;

  return formula;
}

bool ProxyOpportunity::exploit() {
  bool modified = false;

  return modified;
}
// ===========================

} // namespace folio
