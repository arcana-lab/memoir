#include "memoir/support/Casting.hpp"

#include "folio/opportunities/ProxyOpportunity.hpp"

#include "folio/transforms/ProxyManager.hpp"

using namespace llvm::memoir;

namespace folio {

// ===========================
// ProxyOpportunity
template <>
std::string Opportunity::formulate<ProxyOpportunity>(FormulaEnvironment &env) {
  // We don't need to prove anything.
  return "";
}

std::pair<std::string, std::string> ProxyOpportunity::formulate(
    FormulaEnvironment &env) {

  // Unpack the opportunity.
  auto &proxy = this->proxy;

  // Get the id for the proxy.
  auto proxy_id = std::to_string(ProxyManager::get_id(proxy));

  if (auto *natural = dyn_cast<NaturalProxy>(&proxy)) {
    std::string head = "use_proxy(" + proxy_id + ")";

    // We can always use a natural proxy, it has no other effects.
    std::string formula = head + ".\n";

    return std::make_pair(head, formula);

  } else if (auto *artificial = dyn_cast<ArtificialProxy>(&proxy)) {
    // An artificial proxy creates a new collection.
    std::string head = "use_proxy(" + proxy_id + ")";

    // TODO: If we use this proxy, introduce a new collection so that it too can
    // be selected.
    // Get the element type of the proxied contents.
    artificial->proxied().type();

    // std::string formula = "{ " + head + " }. " + " :- " + head;

    std::string formula = "{ " + head + " }.\n";

    return std::make_pair(head, formula);
  }

  MEMOIR_UNREACHABLE("Unhandled Proxy type.");
}

bool ProxyOpportunity::exploit() {
  bool modified = false;

  return modified;
}
// ===========================

} // namespace folio
