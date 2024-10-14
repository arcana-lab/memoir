#ifndef FOLIO_PROXYOPPORTUNITY_H
#define FOLIO_PROXYOPPORTUNITY_H

#include "memoir/ir/Instructions.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "folio/opportunities/Opportunity.hpp"
#include "folio/transforms/Proxy.hpp"

namespace folio {

struct ProxyOpportunity : public Opportunity {
public:
  std::pair<std::string, std::string> formulate(
      FormulaEnvironment &env) override;

  bool exploit(std::function<Selection &(llvm::Value &)> get_selection,
               llvm::ModuleAnalysisManager &MAM) override;

  ProxyOpportunity(
      Proxy &proxy,
      const llvm::memoir::set<llvm::memoir::CollectionAllocInst *> &allocations)
    : proxy(proxy),
      allocations(allocations) {}

protected:
  Proxy &proxy;
  llvm::memoir::set<llvm::memoir::CollectionAllocInst *> allocations;
  llvm::memoir::set<llvm::memoir::CollectionAllocInst *> transients;
};

} // namespace folio

#endif // FOLIO_PROXYOPPORTUNITY_H
