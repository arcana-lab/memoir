#include "common/analysis/TypeAnalysis.hpp"

#include <sstream>

/*
 * TODO: add an actual printer implementation
 */

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const TypeSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const TypeSummary &summary) {
  os << summary.toString();
  return os;
}

std::string StructTypeSummary::toString(std::string indent) const {
  std::string str = "";

  str += "(struct\n";
  for (auto field_type : this->field_types) {
    auto field_str = field_type->toString(indent + "  ");
    str += indent + "  " + field_str + "\n";
  }
  str += indent + ")";

  return str;
}

std::string TensorTypeSummary::toString(std::string indent) const {
  std::string str;

  str = "(tensor\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + "  # of dimensions: " + std::to_string(this->num_dimensions)
         + "\n";
  str += indent + ")";

  return str;
}

std::string StaticTensorTypeSummary::toString(std::string indent) const {
  std::string str;

  str = "(static tensor\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + "  # of dimensions: "
         + std::to_string(this->getNumDimensions()) + "\n";
  for (auto dim = 0; dim < this->length_of_dimensions.size(); dim++) {
    str += indent + "  dimension " + std::to_string(dim) + ": "
           + std::to_string(this->length_of_dimensions.at(dim)) + "\n";
  }
  str += indent + ")";

  return str;
}

std::string AssocArrayTypeSummary::toString(std::string indent) const {
  std::string str;

  str = "(associative array\n";
  str += indent + "  key type: \n";
  str += indent + "    " + this->key_type.toString(indent + "    ") + "\n";
  str += indent + "  value type: \n";
  str += indent + "    " + this->value_type.toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}

std::string AssocArrayTypeSummary::toString(std::string indent) const {
  std::string str;

  str = "(sequence\n";
  str += indent + "  element type: \n";
  str += indent + "    " + this->element_type.toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}

std::string IntegerTypeSummary::toString(std::string indent) const {
  std::string str = "";
  if (!this->is_signed) {
    str += "u";
  }
  str += "int";
  str += std::to_string(this->bitwidth);
  return str;
}

std::string FloatTypeSummary::toString(std::string indent) const {
  return "float";
}

std::string DoubleTypeSummary::toString(std::string indent) const {
  return "double";
}

std::string ReferenceTypeSummary::toString(std::string indent) const {
  std::string str;

  str = "(reference: ";
  if (this->referenced_type.getCode() == TypeCode::StructTy) {
    auto &referenced_struct =
        static_cast<StructTypeSummary &>(this->referenced_type);
    str += referenced_struct.getName() + ")";
  } else {
    str += "\n";
    str += indent + "  " + this->referenced_type.toString(indent + "  ") + "\n";
    str += indent + ")";
  }

  return str;
}

} // namespace llvm::memoir
