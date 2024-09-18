#ifndef FOLIO_PROXYOPPORTUNITY_H
#define FOLIO_PROXYOPPORTUNITY_H

#include "folio/opportunities/Opportunity.hpp"

#include "folio/transforms/Proxy.hpp"

namespace folio {

struct ProxyOpportunity : public Opportunity {
public:
  std::pair<std::string, std::string> formulate(
      FormulaEnvironment &env) override;

  bool exploit() override;

  ProxyOpportunity(Proxy &proxy) : proxy(proxy) {}

protected:
  Proxy &proxy;
};

} // namespace folio

#endif // FOLIO_PROXYOPPORTUNITY_H
