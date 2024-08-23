#include "folio/opportunities/BitmapOpportunity.hpp"

using namespace llvm::memoir;

namespace folio {

template <>
std::string Opportunity::formulate<BitmapOpportunity>(FormulaEnvironment &env) {
  return
      // TODO
      "";
}

std::pair<std::string, std::string> BitmapOpportunity::formulate(
    FormulaEnvironment &env) const {

  std::string formula = "";

  std::string head = "";

  return { head, formula };
}

bool BitmapOpportunity::exploit() {

  bool modified = false;

  return modified;
}

} // namespace folio
