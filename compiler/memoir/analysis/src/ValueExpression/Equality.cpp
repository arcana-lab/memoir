#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include <iostream>
#include <string>

#include "z3++.h"

namespace llvm::memoir {

// Helper function.
std::string get_id(llvm::Value *value,
                   uint32_t &id,
                   map<llvm::Value *, uint32_t> &env) {
  auto found_id = env.find(value);
  if (found_id != env.end()) {
    return std::to_string(found_id->second);
  }
  auto new_id = id++;
  env[value] = new_id;
  return std::to_string(new_id);
}

// Equality
bool check_relation(const ValueExpression &E1,
                    const ValueExpression &E2,
                    std::function<z3::expr(z3::expr, z3::expr)> relation) {
  z3::context c;
  z3::solver s(c);
  z3::expr_vector assumptions(c);
  uint32_t id = 0;
  map<llvm::Value *, uint32_t> env;

  auto x = E1.to_expr(c, s, assumptions, id, env);
  if (!x) {
    infoln("Couldn't make z3 expr for lhs");
    return false;
  }
  auto y = E2.to_expr(c, s, assumptions, id, env);
  if (!y) {
    infoln("Couldn't make z3 expr for rhs");
    return false;
  }
  auto conjecture = relation(*x, *y);

  s.add(!conjecture);

  infoln(Z3_solver_to_string(s.ctx(), s));
  infoln("assuming");
  infoln(assumptions.to_string());
  infoln();
  return s.check(assumptions) == z3::check_result::unsat;
}

bool ValueExpression::operator==(const ValueExpression &E) const {
  if (this->value == E.getValue()) {
    return true;
  }
  return check_relation(*this, E, [](z3::expr a, z3::expr b) -> z3::expr {
    return a == b;
  });
};

bool ValueExpression::operator!=(const ValueExpression &E) const {
  return check_relation(*this, E, [](z3::expr a, z3::expr b) -> z3::expr {
    return a != b;
  });
};

bool ValueExpression::operator<(const ValueExpression &E) const {
  return check_relation(*this, E, [](z3::expr a, z3::expr b) -> z3::expr {
    return a < b;
  });
}

bool ValueExpression::operator<=(const ValueExpression &E) const {
  return check_relation(*this, E, [](z3::expr a, z3::expr b) -> z3::expr {
    return a <= b;
  });
}

bool ValueExpression::operator>(const ValueExpression &E) const {
  return check_relation(*this, E, [](z3::expr a, z3::expr b) -> z3::expr {
    return a > b;
  });
}

bool ValueExpression::operator>=(const ValueExpression &E) const {
  return check_relation(*this, E, [](z3::expr a, z3::expr b) -> z3::expr {
    return a >= b;
  });
}

// LLVM Value pointer-based comparison
bool ValueExpression::operator==(const llvm::Value &Other) const {
  return this->value == &Other;
};

bool ValueExpression::operator!=(const llvm::Value &Other) const {
  return this->value != &Other;
};

opt<z3::expr> ValueExpression::to_expr(
    z3::context &c,
    z3::solver &s,
    z3::expr_vector &assumptions,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  if (this->value) {
    auto var = c.int_const(get_id(this->value, id, env).c_str());
    // auto implication = c.bool_const(std::to_string(id++).c_str());
    // s.add(implies(implication, var > 0));
    // assumptions.push_back(implication);
    return var;
  } else {
    return {};
  }
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
    z3::solver &s,
    z3::expr_vector &assumptions,
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
    z3::solver &s,
    z3::expr_vector &assumptions,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  auto var = c.int_const(get_id(&this->V, id, env).c_str());
  auto implication = c.bool_const(std::to_string(id++).c_str());
  s.add(implies(implication, var > 0));
  assumptions.push_back(implication);
  return var;
}

// ArgumentExpression
bool ArgumentExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, ArgumentExpression);
  return (&(OE.A) == &(this->A));
}

opt<z3::expr> ArgumentExpression::to_expr(
    z3::context &c,
    z3::solver &s,
    z3::expr_vector &assumptions,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  return c.int_const(get_id(&this->A, id, env).c_str());
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
    z3::solver &s,
    z3::expr_vector &assumptions,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {

  switch (this->opcode) {
    default: {
      if (this->I == nullptr) {
        infoln("no instruction for BasicExpression");
        return {};
      }
      auto var = c.int_const(get_id(this->I, id, env).c_str());
      auto implication = c.bool_const(std::to_string(id++).c_str());
      s.add(implies(implication, var > 0));
      assumptions.push_back(implication);
      return var;
    }
    case llvm::Instruction::BinaryOps::Add:
    case llvm::Instruction::BinaryOps::Sub:
    case llvm::Instruction::BinaryOps::Mul:
    case llvm::Instruction::BinaryOps::UDiv:
      break;
  }
  auto *lexpr = this->arguments[0];
  auto *rexpr = this->arguments[1];
  opt<z3::expr> l;
  if (auto *lexpr_as_constant = dyn_cast<ConstantExpression>(lexpr)) {
    if (auto *lexpr_as_const_int =
            dyn_cast<llvm::ConstantInt>(&lexpr_as_constant->C)) {
      l = c.int_val(lexpr_as_const_int->getSExtValue());
    }
  }
  if (!l) {
    l = lexpr->to_expr(c, s, assumptions, id, env);
  }
  auto lhs = l ? *l : c.int_const(get_id(lexpr->getValue(), id, env).c_str());
  if (!l) {
    auto implication = c.bool_const(std::to_string(id++).c_str());
    s.add(implies(implication, lhs > 0));
    assumptions.push_back(implication);
  }
  opt<z3::expr> r;
  if (auto *rexpr_as_constant = dyn_cast<ConstantExpression>(rexpr)) {
    if (auto *rexpr_as_const_int =
            dyn_cast<llvm::ConstantInt>(&rexpr_as_constant->C)) {
      r = c.int_val(rexpr_as_const_int->getSExtValue());
    }
  }
  if (!r) {
    r = rexpr->to_expr(c, s, assumptions, id, env);
  }
  auto rhs = r ? *r : c.int_const(get_id(rexpr->getValue(), id, env).c_str());
  if (!r) {
    auto implication = c.bool_const(std::to_string(id++).c_str());
    s.add(implies(implication, rhs > 0));
    assumptions.push_back(implication);
  }

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

// SelectExpression
opt<z3::expr> SelectExpression::to_expr(
    z3::context &c,
    z3::solver &s,
    z3::expr_vector &assumptions,
    uint32_t &id,
    map<llvm::Value *, uint32_t> &env) const {
  infoln("select to expr");
  if (this->getPHI()) {
    auto var = c.int_const(get_id(this->getPHI(), id, env).c_str());

    auto implication = c.bool_const(std::to_string(id++).c_str());
    s.add(implies(implication, var >= 0));
    assumptions.push_back(implication);

    return var;
  } else {
    return {};
  }
} // namespace llvm::memoir

// PHIExpression
bool PHIExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, PHIExpression);
  // TODO
  return false;
}

// SizeExpression
bool SizeExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, SizeExpression);
  // TODO
  return false;
}

opt<z3::expr> SizeExpression::to_expr(z3::context &c,
                                      z3::solver &s,
                                      z3::expr_vector &assumptions,
                                      uint32_t &id,
                                      map<llvm::Value *, uint32_t> &env) const {
  auto collection_expr = this->CE->to_expr(c, s, assumptions, id, env);

  auto implication = c.bool_const(std::to_string(id++).c_str());
  s.add(implies(implication, collection_expr >= 0));
  assumptions.push_back(implication);

  return collection_expr;
}

// EndExpression
bool EndExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, EndExpression);
  // TODO
  return false;
}

} // namespace llvm::memoir
