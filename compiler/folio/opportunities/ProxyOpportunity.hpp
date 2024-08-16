#ifndef FOLIO_PROXYOPPORTUNITY_H
#define FOLIO_PROXYOPPORTUNITY_H

#include "folio/opportunities/Opportunity.hpp"

namespace folio {

struct ProxyOpportunity : public Opportunity {
public:
  std::string formulate() const override;

  bool exploit() override;

protected:
};

} // namespace folio

#endif // FOLIO_PROXYOPPORTUNITY_H
