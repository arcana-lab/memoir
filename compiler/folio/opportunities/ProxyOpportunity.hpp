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
      const llvm::memoir::Set<llvm::memoir::AllocInst *> &allocations)
    : proxy(proxy),
      allocations(allocations) {}

protected:
  Proxy &proxy;
  llvm::memoir::Set<llvm::memoir::AllocInst *> allocations;
  llvm::memoir::Set<llvm::memoir::AllocInst *> transients;
};

} // namespace folio

#endif // FOLIO_PROXYOPPORTUNITY_H
