#include "memoir/analysis/ValueExpression.hpp"

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const ValueExpression &Expr) {
  os << Expr.toString("");
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const ValueExpression &Expr) {
  os << Expr.toString("");
  return os;
}

std::string ConstantExpression::toString(std::string indent) const {
  std::string llvm_str = "";
  llvm::raw_string_ostream ss(llvm_str);
  ss << C;
  return "constant(" + llvm_str + ")";
}

std::string VariableExpression::toString(std::string indent) const {
  if (V.hasName()) {
    return "variable(" + V.getName().str() + ")";
  }
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

std::string SizeExpression::toString(std::string indent) const {
  return "size";
}

std::string EndExpression::toString(std::string indent) const {
  return "end";
}

} // namespace llvm::memoir
