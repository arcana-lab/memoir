#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include <iostream>

#include "z3++.h"

namespace llvm::memoir {

// Helper function.
const char *get_id(llvm::Value *value,
                   uint32_t &id,
                   map<llvm::Value *, uint32_t> &env) {
  auto found_id = env.find(value);
  if (found_id != env.end()) {
    return std::to_string(found_id->second).c_str();
  }
  auto new_id = id++;
  env[value] = new_id;
  return std::to_string(new_id).c_str();
}

// Equality
bool ValueExpression::operator==(const ValueExpression &E) const {
  return this->equals(E);
};

bool ValueExpression::operator!=(const ValueExpression &E) const {
  return !(*this == E);
};

bool ValueExpression::equals(const llvm::Value &V) const {
  return (this->value == &V);
};

bool ValueExpression::operator==(const llvm::Value &Other) const {
  return this->equals(Other);
};

bool ValueExpression::operator!=(const llvm::Value &Other) const {
  return !(*this == Other);
};

bool ValueExpression::less_than(const ValueExpression &E) const {
  z3::context c;
  uint32_t id = 0;
  map<llvm::Value *, uint32_t> env;

  auto x = this->to_expr(c, id, env);
  if (!x) {
    return false;
  }
  auto y = E.to_expr(c, id, env);
  if (!y) {
    return false;
  }

  z3::solver s(c);
  s.add(*x < *y);
  std::cout << s << "\n";
  return s.check() == z3::check_result::sat;
}

bool ValueExpression::operator<(const ValueExpression &E) const {
  return this->less_than(E);
}

bool ValueExpression::greater_than(const ValueExpression &E) const {
  z3::context c;
  uint32_t id = 0;
  map<llvm::Value *, uint32_t> env;

  auto x = this->to_expr(c, id, env);
  if (!x) {
    return false;
  }
  auto y = E.to_expr(c, id, env);
  if (!y) {
    return false;
  }

  z3::solver s(c);
  s.add(*x > *y);
  std::cout << s << "\n";
  return s.check() == z3::check_result::sat;
}

bool ValueExpression::operator>(const ValueExpression &E) const {
  return this->greater_than(E);
}

opt<z3::expr> ValueExpression::to_expr(
    z3::context &c,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  return {};
}

// Helper macro
#define CHECK_OTHER(OTHER, CLASS)                                              \
  if (!isa<CLASS>(OTHER)) {                                                    \
    return false;                                                              \
  }                                                                            \
  const auto &OE = cast<CLASS>(OTHER);

// ConstantExpression
bool ConstantExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, ConstantExpression);
  return (&(this->C) == &(OE.C));
}

opt<z3::expr> ConstantExpression::to_expr(
    z3::context &c,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  auto *const_int = dyn_cast<llvm::ConstantInt>(&this->getConstant());
  if (!const_int) {
    return {};
  }
  auto const_value = const_int->getZExtValue();
  return c.int_val(const_value);
}

// VariableExpression
bool VariableExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, VariableExpression)
  return (&(this->V) == &(OE.V));
}

opt<z3::expr> VariableExpression::to_expr(
    z3::context &c,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  return c.int_const(get_id(&this->V, id, env));
}

// ArgumentExpression
bool ArgumentExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, ArgumentExpression);
  return (&(OE.A) == &(this->A));
}

opt<z3::expr> ArgumentExpression::to_expr(
    z3::context &c,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  return c.int_const(get_id(&this->A, id, env));
}

// UnknownExpression
bool UnknownExpression::equals(const ValueExpression &E) const {
  return false;
}

// BasicExpressionpression
bool BasicExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, BasicExpression);
  if ((this->opcode != E.opcode) || (this->getLLVMType() != OE.getLLVMType())
      || (this->getMemOIRType() != OE.getMemOIRType())
      || (this->I->getNumOperands() != OE.I->getNumOperands())) {
    return false;
  }
  return false;
};

opt<z3::expr> BasicExpression::to_expr(
    z3::context &c,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {

  switch (this->opcode) {
    default:
      return {};
    case llvm::Instruction::BinaryOps::Add:
    case llvm::Instruction::BinaryOps::Sub:
    case llvm::Instruction::BinaryOps::Mul:
    case llvm::Instruction::BinaryOps::UDiv:
      break;
  }
  auto *lexpr = this->arguments[0];
  auto *rexpr = this->arguments[1];
  auto l = lexpr->to_expr(c, id, env);
  auto lhs = l ? *l : c.int_const(get_id(lexpr->getValue(), id, env));
  auto r = rexpr->to_expr(c, id, env);
  auto rhs = r ? *r : c.int_const(get_id(rexpr->getValue(), id, env));

  switch (this->opcode) {
    case llvm::Instruction::BinaryOps::Add:
      return lhs + rhs;
    case llvm::Instruction::BinaryOps::Sub:
      return lhs - rhs;
    case llvm::Instruction::BinaryOps::Mul:
      return lhs * rhs;
    case llvm::Instruction::BinaryOps::UDiv:
      return lhs / rhs;
    default:
      return {};
  }
}

// PHIExpression
bool PHIExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, PHIExpression);
  // TODO
  return false;
}

// CollectionExpression
bool CollectionExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, CollectionExpression);
  // TODO
  return false;
}

// StructExpression
bool StructExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, StructExpression);
  // TODO
  return false;
}

// SizeExpression
bool SizeExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, SizeExpression);
  // TODO
  return false;
}

} // namespace llvm::memoir
