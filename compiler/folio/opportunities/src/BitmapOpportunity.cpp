#include "folio/opportunities/BitmapOpportunity.hpp"

#include "folio/transforms/ProxyManager.hpp"

using namespace llvm::memoir;

namespace folio {

template <>
std::string Opportunity::formulate<BitmapOpportunity>(FormulaEnvironment &env) {
  auto size_type = Type::get_size_type(env.module().getDataLayout());
  auto size_type_str = size_type.get_code().value_or("u64");

  std::string formula =
      // The key type of a bitmap is the size type.
      "keytype(C, " + size_type_str + ") :- use_bitmap(C), collection(C)." +
      // The bitmap _can_ be a sequence implementation.
      "{seq(C)} :- use_bitmap(C), collection(C).";

  return formula;
}

std::pair<std::string, std::string> BitmapOpportunity::formulate(
    FormulaEnvironment &env) {

  // Get an ID for the allocation's identifier.
  auto alloc_id = std::to_string(env.get_id(this->allocation.getCallInst()));

  std::string head = "use_bitmap(" + alloc_id + ")";

  // We can use the bitmap opportunity if its proxy is used.
  std::string formula = "{ " + head + " }";
  if (this->uses_proxy) {
    auto proxy_id = std::to_string(ProxyManager::get_id(*this->uses_proxy));
    formula += " :- use_proxy(" + proxy_id + ").\n";
  } else {
    formula += ".\n";
  }

  return { head, formula };
}

bool BitmapOpportunity::exploit() {

  bool modified = false;

  return modified;
}

} // namespace folio
