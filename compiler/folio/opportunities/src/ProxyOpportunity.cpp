#include "folio/opportunities/ProxyOpportunity.hpp"

using namespace llvm::memoir;

namespace folio {

// ===========================
// ProxyOpportunity
template <>
std::string Opportunity::formulate<ProxyOpportunity>(FormulaEnvironment &env) {
  return
      // Declare the opportunity for A to use B as a proxy of C.
      "{ useProxyOpportunity(A, B, C) } :- "
      "collection(A, assoc, KT, A_VT). type(A_VT). "
      "collection(B, seq, index, B_VT). type(B_VT). "
      "collection(C, assoc, KT, C_VT). type(C_VT). "
      "type(KT). "
      "derivative(A, C). "
      "proxy(A, B). "
      "\n"
      // Declare that, if the proxy opportunity is used, the implementation of A
      // changes.
      "seq(A) :- "
      "useProxyOpportunity(A, B, C). "
      "seq(B). collection(C). ";
}

std::pair<std::string, std::string> ProxyOpportunity::formulate(
    FormulaEnvironment &env) const {

  std::string formula = "";

  std::string head = "";

  return std::make_pair(head, formula);
}

bool ProxyOpportunity::exploit() {
  bool modified = false;

  return modified;
}
// ===========================

} // namespace folio
