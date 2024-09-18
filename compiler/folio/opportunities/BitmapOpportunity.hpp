#ifndef FOLIO_BITMAP_OPPORTUNITY
#define FOLIO_BITMAP_OPPORTUNITY

#include "memoir/ir/Instructions.hpp"

#include "folio/opportunities/Opportunity.hpp"

#include "folio/transforms/Proxy.hpp"

namespace folio {

struct BitmapOpportunity : public Opportunity {
public:
  BitmapOpportunity(llvm::memoir::AssocAllocInst &allocation,
                    Proxy *proxy = nullptr)
    : allocation(allocation),
      uses_proxy(proxy) {}

  std::pair<std::string, std::string> formulate(
      FormulaEnvironment &env) override;

  bool exploit() override;

protected:
  llvm::memoir::AssocAllocInst &allocation;
  Proxy *uses_proxy;
};

} // namespace folio

#endif // FOLIO_BITMAP_OPPORTUNITY
