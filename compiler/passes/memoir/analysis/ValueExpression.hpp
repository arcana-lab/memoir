#ifndef MEMOIR_VALUEEXPRESSION_H
#define MEMOIR_VALUEEXPRESSION_H
#pragma once

// LLVM
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "llvm/Transforms/Utils/Cloning.h"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/support/Casting.hpp"

/*
 * This file contains the Expression class for use in ValueNumbering as an
 * analysis output.
 *
 * Author(s): Tommy McMichen
 * Created: April 4, 2023
 */

namespace llvm::memoir {

enum ExpressionKind {
  EK_Base,
  EK_Constant,
  EK_Variable,
  EK_Argument,
  EK_Unknown,
  EK_BasicStart,
  EK_Basic,
  EK_Cast,
  EK_ICmp,
  EK_PHI,
  EK_Select,
  EK_Call,
  EK_BasicEnd,
  EK_MemOIRStart,
  EK_MemOIR,
  EK_Collection,
  EK_Struct,
  EK_Slice,
  EK_Size,
  EK_MemOIREnd
};

struct ValueExpression {
public:
  ValueExpression(ExpressionKind EK = EK_Base,
                  llvm::Value *value = nullptr,
                  unsigned opcode = 0,
                  bool commutative = false,
                  bool is_memoir = false)
    : EK(EK),
      opcode(opcode),
      value(value),
      commutative(commutative),
      is_memoir(is_memoir),
      arguments({}) {}

  ExpressionKind getKind() const {
    return this->EK;
  };

  // Equality.
  virtual bool equals(const ValueExpression &E) const = 0;
  bool operator==(const ValueExpression &E) const {
    return this->equals(E);
  };
  bool operator!=(const ValueExpression &E) const {
    return !(*this == E);
  };
  bool equals(const llvm::Value &V) const {
    return (this->value == &V);
  };
  bool operator==(const llvm::Value &Other) const {
    return this->equals(Other);
  };
  bool operator!=(const llvm::Value &Other) const {
    return !(*this == Other);
  };

  // Accessors.
  llvm::Value *getValue() const {
    return this->value;
  }

  virtual llvm::Type *getLLVMType() const {
    if (!this->value) {
      return this->arguments.at(0)->getLLVMType();
    }
    return this->value->getType();
  }

  Type *getMemOIRType() const {
    // TODO
    return nullptr;
  }

  unsigned getNumArguments() const {
    return this->arguments.size();
  }

  ValueExpression *getArgument(unsigned idx) const {
    MEMOIR_ASSERT((idx < this->getNumArguments()),
                  "Index out of range for ValueExpression arguments");
    return this->arguments[idx];
  }

  void setArgument(unsigned idx, ValueExpression &expr) {
    this->arguments[idx] = &expr;
  }

  // Availability.
  virtual bool isAvailable(llvm::Instruction &IP,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) = 0;

  // Materialization.
  virtual llvm::Value *materialize(llvm::Instruction &IP,
                                   MemOIRBuilder *builder = nullptr,
                                   const llvm::DominatorTree *DT = nullptr,
                                   llvm::CallBase *call_context = nullptr) = 0;

  // These functions can be used to query information about possible function
  // versioning if it was necessary.
  llvm::Function *getVersionedFunction() {
    return this->function_version;
  };

  llvm::ValueToValueMapTy *getValueMapping() {
    return this->version_mapping;
  };

  llvm::CallBase *getVersionedCall() {
    return this->call_version;
  };

  // Debug.
  virtual std::string toString(std::string indent = "") const = 0;
  friend std::ostream &operator<<(std::ostream &os,
                                  const ValueExpression &Expr) {
    os << Expr.toString("");
    return os;
  }
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const ValueExpression &Expr) {
    os << Expr.toString("");
    return os;
  }

  static llvm::Argument *handleCallContext(
      ValueExpression &Expr,
      llvm::Value &materialized_value,
      llvm::CallBase &call_context,
      MemOIRBuilder *builder = nullptr,
      const llvm::DominatorTree *DT = nullptr);

  // State.
  ExpressionKind EK;
  llvm::Value *value;
  unsigned opcode;
  vector<ValueExpression *> arguments;
  bool is_memoir;
  bool commutative;

  // Materialization state.
  llvm::Function *function_version;
  llvm::ValueToValueMapTy *version_mapping;
  llvm::CallBase *call_version;
};

#define CHECK_OTHER(OTHER, CLASS)                                              \
  if (!isa<CLASS>(OTHER)) {                                                    \
    return false;                                                              \
  }                                                                            \
  const auto &OE = cast<CLASS>(OTHER);

struct ConstantExpression : public ValueExpression {
public:
  ConstantExpression(llvm::Constant &C)
    : ValueExpression(EK_Constant, &C),
      C(C) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_Constant);
  }

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, ConstantExpression);
    return (&(this->C) == &(OE.C));
  }

  llvm::Constant &getConstant() const {
    return C;
  }

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  llvm::Constant &C;
};

struct VariableExpression : public ValueExpression {
public:
  VariableExpression(llvm::Value &V) : ValueExpression(EK_Variable, &V), V(V) {
    println("VariableExpression is minimally defined "
            "as we don't have need of it for now.");
  }

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_Variable);
  }

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, VariableExpression)
    return (&(this->V) == &(OE.V));
  }

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  llvm::Value &V;
};

struct ArgumentExpression : public ValueExpression {
public:
  ArgumentExpression(llvm::Argument &A)
    : ValueExpression(EK_Argument, &A),
      A(A) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_Argument);
  }

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, ArgumentExpression);
    return (&(OE.A) == &(this->A));
  }

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  llvm::Argument &A;
};

struct UnknownExpression : public ValueExpression {
public:
  UnknownExpression() : ValueExpression(EK_Unknown) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_Unknown);
  }

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, UnknownExpression);
    // TODO
    return false;
  }

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;
};

struct BasicExpression : public ValueExpression {
public:
  BasicExpression(ExpressionKind EK, unsigned opcode)
    : ValueExpression(EK, nullptr, opcode),
      I(nullptr) {}
  BasicExpression(unsigned opcode) : BasicExpression(EK_Basic, opcode) {}
  BasicExpression(ExpressionKind EK, llvm::Instruction &I)
    : ValueExpression(EK, &I, I.getOpcode()),
      I(&I) {}
  BasicExpression(llvm::Instruction &I) : BasicExpression(EK_Basic, I) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() > EK_BasicStart) && (E->getKind() < EK_BasicEnd);
  };

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, BasicExpression);
    if ((this->opcode != E.opcode) || (this->getLLVMType() != OE.getLLVMType())
        || (this->getMemOIRType() != OE.getMemOIRType())
        || (this->I->getNumOperands() != OE.I->getNumOperands())) {
      return false;
    }
    return false;
  };

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  // Borrowed State.
  llvm::Instruction *I;
};

struct CastExpression : public BasicExpression {
public:
  CastExpression(llvm::CastInst &I)
    : BasicExpression(EK_Cast, I),
      dest_type(*I.getDestTy()) {}
  CastExpression(unsigned opcode, ValueExpression &expr, llvm::Type &dest_type)
    : BasicExpression(EK_Cast, opcode),
      dest_type(dest_type) {
    this->arguments.push_back(&expr);
  }

  ValueExpression &getOperand() const {
    return *(this->arguments.at(0));
  };

  llvm::Type &getDestType() const {
    return this->dest_type;
  };

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  // Borrowed state.
  llvm::Type &dest_type;
};

struct ICmpExpression : public BasicExpression {
public:
  ICmpExpression(llvm::ICmpInst &cmp)
    : BasicExpression(EK_ICmp, cmp),
      predicate(cmp.getPredicate()) {}
  ICmpExpression(llvm::CmpInst::Predicate pred,
                 ValueExpression &LHS,
                 ValueExpression &RHS)
    : BasicExpression(EK_ICmp, (Instruction::ICmp << 8) | pred),
      predicate(pred) {
    this->arguments.push_back(&LHS);
    this->arguments.push_back(&RHS);
  }

  llvm::CmpInst::Predicate getPredicate() const {
    return this->predicate;
  }

  ValueExpression *getLHS() const {
    return this->arguments.at(0);
  }

  ValueExpression *getRHS() const {
    return this->arguments.at(1);
  }

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  // Owned state.
  llvm::CmpInst::Predicate predicate;
};

struct PHIExpression : public BasicExpression {
public:
  PHIExpression(llvm::PHINode &phi) : BasicExpression(EK_PHI, phi), phi(phi) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_PHI);
  }

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, PHIExpression);
    // TODO
    return false;
  }

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  llvm::PHINode &phi;
};

struct SelectExpression : public BasicExpression {
public:
  SelectExpression(llvm::SelectInst &select)
    : BasicExpression(EK_Select, select) {}
  SelectExpression(ValueExpression &condition,
                   ValueExpression &true_value,
                   ValueExpression &false_value)
    : BasicExpression(EK_Select, Instruction::Select) {
    this->arguments.push_back(&condition);
    this->arguments.push_back(&true_value);
    this->arguments.push_back(&false_value);
  }

  ValueExpression *getCondition() const {
    return this->getArgument(0);
  }

  ValueExpression *getTrueValue() const {
    return this->getArgument(1);
  }

  ValueExpression *getFalseValue() const {
    return this->getArgument(2);
  }

  llvm::Type *getLLVMType() const override {
    return this->getTrueValue()->getLLVMType();
  }

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;
};

struct CallExpression : public BasicExpression {
public:
  CallExpression(llvm::CallInst &call)
    : BasicExpression(EK_Call, call),
      call(call) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_Call);
  }

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  llvm::CallInst &call;
};

struct MemOIRExpression : public ValueExpression {
public:
  MemOIRExpression(ExpressionKind EK) : ValueExpression(EK) {}
  MemOIRExpression() : MemOIRExpression(EK_MemOIR) {}

  static bool classof(const ValueExpression *E) {
    return ((E->getKind() == EK_MemOIRStart) && (E->getKind() == EK_MemOIREnd));
  }
};

struct CollectionExpression : public MemOIRExpression {
public:
  CollectionExpression(Collection &C) : MemOIRExpression(EK_Collection), C(C) {}

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, CollectionExpression);
    // TODO
    return false;
  }

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  Collection &C;
};

struct StructExpression : public MemOIRExpression {
public:
  StructExpression(Struct &S) : MemOIRExpression(EK_Struct), S(S) {}

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, StructExpression);
    // TODO
    return false;
  }

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  Struct &S;
};

struct SliceExpression : public MemOIRExpression {
public:
  SliceExpression(SliceInst &I) : MemOIRExpression(EK_Slice), I(&I) {}
  SliceExpression(CollectionExpression &C,
                  ValueExpression &left,
                  ValueExpression &right)
    : MemOIRExpression(EK_Slice),
      CE(&C),
      left_index(&left),
      right_index(&right) {}

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  SliceInst *I;
  CollectionExpression *CE;
  ValueExpression *left_index;
  ValueExpression *right_index;
};

struct SizeExpression : public MemOIRExpression {
public:
  SizeExpression(CollectionExpression *CE)
    : MemOIRExpression(EK_Size),
      CE(CE) {}
  SizeExpression() : SizeExpression(nullptr) {}

  bool equals(const ValueExpression &E) const override {
    CHECK_OTHER(E, SizeExpression);
    // TODO
    return false;
  }

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  CollectionExpression *CE;
};

// Casting.
template <typename LLVMTy>
// std::enable_if<std::is_base_of<llvm::Value, LLVMTy>,
//                std::add_pointer<LLVMTy>>::type
LLVMTy *as(ValueExpression *E) {
  if (!E) {
    return nullptr;
  }
  return dyn_cast_or_null<LLVMTy>(E->getValue());
};

template <typename LLVMTy>
// std::enable_if<std::is_base_of<llvm::Value, LLVMTy>,
//                std::add_pointer<LLVMTy>>::type
LLVMTy *as(ValueExpression &E) {
  return dyn_cast_or_null<LLVMTy>(E.getValue());
};

} // namespace llvm::memoir

#endif
