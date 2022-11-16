#include "common/analysis/CollectionAnalysis.hpp"

#include <sstream>

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const CollectionSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const CollectionSummary &summary) {
  os << summary.toString();
  return os;
}

/*
 * BaseCollectionSummary implementation
 */
std::string BaseCollectionSummary::toString(std::string indent = "") const {
  return "base collection";
}

/*
 * FieldArraySummary implementation
 */
std::string FieldArraySummary::toString(std::string indent = "") const {
  return "field array";
}

/*
 * ControlPHISummary implementationn
 */
std::string ControlPHISummary::toString(std::string indent = "") const {
  return "control PHI";
}

/*
 * DefPHISummary implementation
 */
std::string DefPHISummary::toString(std::string indent = "") const {
  return "def PHI";
}

/*
 * UsePHISummary implementation
 */
std::string UsePHISummary::toString(std::string indent = "") const {
  return "use PHI";
}

} // namespace llvm::memoir
