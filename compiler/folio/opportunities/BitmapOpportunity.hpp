#ifndef FOLIO_BITMAP_OPPORTUNITY
#define FOLIO_BITMAP_OPPORTUNITY

#include "folio/opportunities/Opportunity.hpp"

namespace folio {

struct BitmapOpportunity : public Opportunity {
public:
  std::pair<std::string, std::string> formulate(
      FormulaEnvironment &env) const override;

  bool exploit() override;

protected:
};

} // namespace folio

#endif // FOLIO_BITMAP_OPPORTUNITY
