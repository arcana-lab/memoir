#include "memoir/analysis/ValueExpression.hpp"

namespace llvm::memoir {

std::string ConstantExpression::toString(std::string indent) const {
  return "constant";
}

std::string VariableExpression::toString(std::string indent) const {
  return "variable";
}

std::string ArgumentExpression::toString(std::string indent) const {
  return "argument";
}

std::string UnknownExpression::toString(std::string indent) const {
  return "unknown";
}

std::string BasicExpression::toString(std::string indent) const {
  return "basic";
}

std::string CastExpression::toString(std::string indent) const {
  return "cast";
}

std::string ICmpExpression::toString(std::string indent) const {
  return "integer cmp";
}

std::string PHIExpression::toString(std::string indent) const {
  return "phi";
}

std::string SelectExpression::toString(std::string indent) const {
  return "select";
}

std::string CallExpression::toString(std::string indent) const {
  return "call";
}

std::string CollectionExpression::toString(std::string indent) const {
  return "collection";
}

std::string StructExpression::toString(std::string indent) const {
  return "struct";
}

std::string SliceExpression::toString(std::string indent) const {
  return "slice";
}

std::string SizeExpression::toString(std::string indent) const {
  return "size";
}

} // namespace llvm::memoir
