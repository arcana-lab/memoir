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
#include "memoir/support/InternalDatatypes.hpp"

/*
 * This file contains the Expression class for use in ValueNumbering as an
 * analysis output.
 * TODO: Move availability analysis out of this class hierarchy and into its
 * own.
 * TODO: Move materialization out of this class hierarchy and into its own
 * (possibly with availability).
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
  EK_Slice,
  EK_Size,
  EK_End,
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
      is_memoir(is_memoir) {}

  virtual ~ValueExpression() = default;

  ExpressionKind getKind() const;

  // Equality.
  virtual bool equals(const ValueExpression &E) const = 0;

  // Accessors.
  llvm::Value *getValue() const;
  virtual llvm::Type *getLLVMType() const;
  Type *getMemOIRType() const;
  unsigned getNumArguments() const;
  ValueExpression *getArgument(unsigned idx) const;
  void setArgument(unsigned idx, ValueExpression &expr);

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
  llvm::Function *getVersionedFunction();
  llvm::ValueToValueMapTy *getValueMapping();
  llvm::CallBase *getVersionedCall();

  // Debug.
  virtual std::string toString(std::string indent = "") const = 0;
  friend std::ostream &operator<<(std::ostream &os,
                                  const ValueExpression &Expr);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const ValueExpression &Expr);

  // Internal helper function.
  static llvm::Argument *handleCallContext(
      ValueExpression &Expr,
      llvm::Value &materialized_value,
      llvm::CallBase &call_context,
      MemOIRBuilder *builder = nullptr,
      const llvm::DominatorTree *DT = nullptr);

  // State.
  ExpressionKind EK;
  unsigned opcode;
  llvm::Value *value;
  vector<ValueExpression *> arguments;
  bool commutative;
  bool is_memoir;

  // Materialization state.
  llvm::Function *function_version;
  llvm::ValueToValueMapTy *version_mapping;
  llvm::CallBase *call_version;
};

struct ConstantExpression : public ValueExpression {
public:
  ConstantExpression(llvm::Constant &C)
    : ValueExpression(EK_Constant, &C),
      C(C) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_Constant);
  }

  bool equals(const ValueExpression &E) const override;

  llvm::Constant &getConstant() const;

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

  bool equals(const ValueExpression &E) const override;

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

  bool equals(const ValueExpression &E) const override;

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

  bool equals(const ValueExpression &E) const override;

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
  BasicExpression(unsigned opcode, vector<ValueExpression *> operands)
    : BasicExpression(EK_Basic, opcode) {
    this->arguments.reserve(operands.size());
    for (auto *operand : operands) {
      this->arguments.push_back(operand);
    }
  }
  BasicExpression(unsigned opcode,
                  std::initializer_list<ValueExpression *> operands)
    : BasicExpression(opcode, vector<ValueExpression *>(operands)) {}

  BasicExpression(ExpressionKind EK, llvm::Instruction &I)
    : ValueExpression(EK, &I, I.getOpcode()),
      I(&I) {}
  BasicExpression(llvm::Instruction &I) : BasicExpression(EK_Basic, I) {}

  static bool classof(const ValueExpression *E) {
    return (E->getKind() > EK_BasicStart) && (E->getKind() < EK_BasicEnd);
  };

  bool equals(const ValueExpression &E) const override;

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

  ValueExpression &getOperand() const;

  llvm::Type &getDestType() const;

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
    : BasicExpression(EK_ICmp, (llvm::Instruction::ICmp << 8) | pred),
      predicate(pred) {
    this->arguments.push_back(&LHS);
    this->arguments.push_back(&RHS);
  }

  llvm::CmpInst::Predicate getPredicate() const;
  ValueExpression *getLHS() const;
  ValueExpression *getRHS() const;

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
  PHIExpression(llvm::PHINode &phi) : BasicExpression(EK_PHI, phi), phi(&phi) {}
  PHIExpression(vector<ValueExpression *> incoming_expressions,
                vector<llvm::BasicBlock *> incoming_blocks)
    : BasicExpression(EK_PHI),
      incoming_blocks(incoming_blocks) {
    // Add the incoming expressions to the expressio argument list.
    for (auto incoming_expr : incoming_expressions) {
      this->arguments.push_back(incoming_expr);
    }
  }

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_PHI);
  }

  bool equals(const ValueExpression &E) const override;

  llvm::BasicBlock &getIncomingBlock(unsigned index) const;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  // Realized
  llvm::PHINode *phi;

  // Unrealized
  vector<llvm::BasicBlock *> incoming_blocks;
};

struct SelectExpression : public BasicExpression {
public:
  SelectExpression(llvm::SelectInst &select)
    : BasicExpression(EK_Select, select) {}
  SelectExpression(ValueExpression *condition,
                   ValueExpression *true_value,
                   ValueExpression *false_value)
    : BasicExpression(EK_Select, Instruction::Select) {
    this->arguments.push_back(condition);
    this->arguments.push_back(true_value);
    this->arguments.push_back(false_value);
  }
  SelectExpression(llvm::PHINode &phi)
    : BasicExpression(EK_Select, Instruction::Select),
      phi(&phi) {}
  SelectExpression() : BasicExpression(EK_Select, Instruction::Select) {}

  ValueExpression *getCondition() const;
  ValueExpression *getTrueValue() const;
  ValueExpression *getFalseValue() const;
  llvm::Type *getLLVMType() const override;
  llvm::PHINode *getPHI() const;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  llvm::PHINode *phi;
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

struct SizeExpression : public MemOIRExpression {
public:
  SizeExpression(ValueExpression *CE) : MemOIRExpression(EK_Size), CE(CE) {}
  SizeExpression() : SizeExpression(nullptr) {}

  bool equals(const ValueExpression &E) const override;

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_Size);
  }

  ValueExpression *CE;
};

struct EndExpression : public MemOIRExpression {
public:
  EndExpression() : MemOIRExpression(EK_End) {}

  bool equals(const ValueExpression &E) const override;

  bool isAvailable(llvm::Instruction &IP,
                   const llvm::DominatorTree *DT = nullptr,
                   llvm::CallBase *call_context = nullptr) override;

  llvm::Value *materialize(llvm::Instruction &IP,
                           MemOIRBuilder *builder = nullptr,
                           const llvm::DominatorTree *DT = nullptr,
                           llvm::CallBase *call_context = nullptr) override;

  std::string toString(std::string indent = "") const override;

  static bool classof(const ValueExpression *E) {
    return (E->getKind() == EK_End);
  }
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
