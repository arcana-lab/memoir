#ifndef MEMOIR_INSTRUCTIONS_H
#define MEMOIR_INSTRUCTIONS_H

#include <cstddef>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/ir/Keywords.hpp"
#include "memoir/ir/Types.hpp"

/*
 * MemOIR Instructions and a wrapper of an LLVM Instruction.
 *
 * Author(s): Tommy McMichen
 * Created: December 13, 2022
 */

namespace llvm::memoir {

struct CollectionType;

// Abstract MemOIR Instruction
struct MemOIRInst {
public:
  static MemOIRInst *get(llvm::Instruction &I);
  static bool is_mutator(MemOIRInst &I);
  static void invalidate();

  llvm::Function &getCalledFunction() const;

  llvm::Module *getModule() const;
  llvm::Function *getFunction() const;
  llvm::BasicBlock *getParent() const;
  llvm::CallInst &getCallInst() const;

  MemOIR_Func getKind() const;

  bool has_keywords() const;

  template <typename KeywordTy>
  std::optional<KeywordTy> get_keyword() const;

  llvm::iterator_range<keyword_iterator> keywords() const;
  keyword_iterator kw_begin() const;
  keyword_iterator kw_end() const;

  explicit operator llvm::Value *() {
    return &this->getCallInst();
  }
  explicit operator llvm::Value &() {
    return this->getCallInst();
  }
  explicit operator llvm::Instruction *() {
    return &this->getCallInst();
  }
  explicit operator llvm::Instruction &() {
    return this->getCallInst();
  }

  friend std::ostream &operator<<(std::ostream &os, const MemOIRInst &I);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const MemOIRInst &I);
  virtual std::string toString() const = 0;

  virtual ~MemOIRInst() = default;

protected:
  llvm::CallInst &call_inst;

  static map<llvm::Instruction *, MemOIRInst *> *llvm_to_memoir;

  MemOIRInst(llvm::CallInst &call_inst) : call_inst(call_inst) {}
};

// Types.
struct TypeInst : public MemOIRInst {
public:
  virtual Type &getType() const = 0;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS)                                    \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  TypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

struct UInt64TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT64_TYPE);
  };

  std::string toString() const override;

protected:
  UInt64TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct UInt32TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT32_TYPE);
  };

  std::string toString() const override;

protected:
  UInt32TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct UInt16TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT16_TYPE);
  };

  std::string toString() const override;

protected:
  UInt16TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct UInt8TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT8_TYPE);
  };

  std::string toString() const override;

protected:
  UInt8TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct UInt2TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT2_TYPE);
  };

  std::string toString() const override;

protected:
  UInt2TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct Int64TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT64_TYPE);
  };

  std::string toString() const override;

protected:
  Int64TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct Int32TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT32_TYPE);
  };

  std::string toString() const override;

protected:
  Int32TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct Int16TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT16_TYPE);
  };

  std::string toString() const override;

protected:
  Int16TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct Int8TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT8_TYPE);
  };

  std::string toString() const override;

protected:
  Int8TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct Int2TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT2_TYPE);
  };

  std::string toString() const override;

protected:
  Int2TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct BoolTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::BOOL_TYPE);
  };

  std::string toString() const override;

protected:
  BoolTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct FloatTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::FLOAT_TYPE);
  };

  std::string toString() const override;

protected:
  FloatTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct DoubleTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DOUBLE_TYPE);
  };

  std::string toString() const override;

protected:
  DoubleTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct PointerTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::POINTER_TYPE);
  };

  std::string toString() const override;

protected:
  PointerTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct VoidTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::VOID_TYPE);
  };

  std::string toString() const override;

protected:
  VoidTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct ReferenceTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getReferencedType() const;
  llvm::Value &getReferencedTypeOperand() const;
  llvm::Use &getReferencedTypeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::REFERENCE_TYPE);
  };

  std::string toString() const override;

protected:
  ReferenceTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

#if 0
struct DefineTypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string getName() const;
  llvm::Value &getNameOperand() const;
  llvm::Use &getNameOperandAsUse() const;

  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DEFINE_TYPE);
  };

  std::string toString() const override;

protected:
  DefineTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct LookupTypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string getName() const;
  llvm::Value &getNameOperand() const;
  llvm::Use &getNameOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::LOOKUP_TYPE);
  };

  std::string toString() const override;

protected:
  LookupTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};
#endif

struct TupleTypeInst : public TypeInst {
public:
  Type &getType() const override;

  unsigned getNumberOfFields() const;
  llvm::Value &getNumberOfFieldsOperand() const;
  llvm::Use &getNumberOfFieldsOperandAsUse() const;

  Type &getFieldType(unsigned field_index) const;
  llvm::Value &getFieldTypeOperand(unsigned field_index) const;
  llvm::Use &getFieldTypeOperandAsUse(unsigned field_index) const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::TUPLE_TYPE);
  };

  std::string toString() const override;

protected:
  TupleTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct ArrayTypeInst : public TypeInst {
public:
  Type &getType() const override;

  Type &getElementType() const;
  llvm::Value &getElementTypeOperand() const;
  llvm::Use &getElementTypeOperandAsUse() const;

  size_t getLength() const;
  llvm::Value &getLengthOperand() const;
  llvm::Use &getLengthOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ARRAY_TYPE);
  };

  std::string toString() const override;

protected:
  ArrayTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

struct AssocTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getKeyType() const;
  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;
  Type &getValueType() const;
  llvm::Value &getValueOperand() const;
  llvm::Use &getValueOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSOC_ARRAY_TYPE);
  };

  std::string toString() const override;

protected:
  AssocTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};
using AssocArrayTypeInst = struct AssocTypeInst;

struct SequenceTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getElementType() const;
  llvm::Value &getElementOperand() const;
  llvm::Use &getElementOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SEQUENCE_TYPE);
  };

  std::string toString() const override;

protected:
  SequenceTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst) {}

  friend struct MemOIRInst;
};

// Allocations
struct AllocInst : public MemOIRInst {
public:
  llvm::Value &getAllocation() const;

  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  using size_iterator = llvm::User::value_op_iterator;
  llvm::iterator_range<size_iterator> sizes();
  size_iterator sizes_begin();
  size_iterator sizes_end();

  using const_size_iterator = llvm::User::const_value_op_iterator;
  llvm::iterator_range<const_size_iterator> sizes() const;
  const_size_iterator sizes_begin() const;
  const_size_iterator sizes_end() const;

  using size_op_iterator = llvm::User::op_iterator;
  llvm::iterator_range<size_op_iterator> size_operands();
  size_op_iterator size_ops_begin();
  size_op_iterator size_ops_end();

  using const_size_op_iterator = llvm::User::const_op_iterator;
  llvm::iterator_range<const_size_op_iterator> size_operands() const;
  const_size_op_iterator size_ops_begin() const;
  const_size_op_iterator size_ops_end() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_ALLOC_INST(ENUM, FUNC, CLASS)                                   \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString() const override;

protected:
  AllocInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

// Accesses
struct AccessInst : public MemOIRInst {
public:
  Type &getObjectType() const;
  Type &getElementType() const;

  virtual llvm::Value &getObject() const = 0;
  virtual llvm::Use &getObjectAsUse() const = 0;

  using index_iterator = llvm::User::value_op_iterator;
  llvm::iterator_range<index_iterator> indices();
  index_iterator indices_begin();
  index_iterator indices_end();

  using const_index_iterator = llvm::User::const_value_op_iterator;
  llvm::iterator_range<const_index_iterator> indices() const;
  const_index_iterator indices_begin() const;
  const_index_iterator indices_end() const;

  using index_op_iterator = llvm::User::op_iterator;
  llvm::iterator_range<index_op_iterator> index_operands();
  index_op_iterator index_operands_begin();
  index_op_iterator index_operands_end();

  using const_index_op_iterator = llvm::User::const_op_iterator;
  llvm::iterator_range<const_index_op_iterator> index_operands() const;
  const_index_op_iterator index_operands_begin() const;
  const_index_op_iterator index_operands_end() const;

  static bool classof(const MemOIRInst *I) {
    return (
#define HANDLE_ACCESS_INST(ENUM, FUNC, CLASS)                                  \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false);
  };

protected:
  AccessInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

struct ReadInst : public AccessInst {
public:
  llvm::Value &getValueRead() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_READ_INST(ENUM, FUNC, CLASS)                                    \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString() const override;

protected:
  ReadInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

/**
 * GetInst is unstable!
 */
struct GetInst : public AccessInst {
public:
  llvm::Value &getNestedObject() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return I->getKind() == MemOIR_Func::GET;
  };

  std::string toString() const override;

protected:
  GetInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

struct CopyInst : public AccessInst {
  llvm::Value &getResult() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return I->getKind() == MemOIR_Func::COPY;
  }

  std::string toString() const override;

protected:
  CopyInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

struct SizeInst : public AccessInst {
public:
  llvm::Value &getSize() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return I->getKind() == MemOIR_Func::SIZE;
  };

  std::string toString() const override;

protected:
  SizeInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

// Instructions that apply functional updates to an object
struct UpdateInst : public AccessInst {
public:
  llvm::Value &getResult() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_UPDATE_INST(ENUM, FUNC, CLASS)                                  \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  }

protected:
  UpdateInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

struct WriteInst : public UpdateInst {
public:
  llvm::Value &getResult() const;

  llvm::Value &getValueWritten() const;
  llvm::Use &getValueWrittenAsUse() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS)                                   \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString() const override;

protected:
  WriteInst(llvm::CallInst &call_inst) : UpdateInst(call_inst) {}

  friend struct MemOIRInst;
};

struct InsertInst : public UpdateInst {
public:
  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return I->getKind() == MemOIR_Func::INSERT;
  }

  std::string toString() const override;

protected:
  InsertInst(llvm::CallInst &call_inst) : UpdateInst(call_inst) {}

  friend struct MemOIRInst;
};

struct RemoveInst : public UpdateInst {
public:
  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return I->getKind() == MemOIR_Func::REMOVE;
  };

  std::string toString() const override;

protected:
  RemoveInst(llvm::CallInst &call_inst) : UpdateInst(call_inst) {}

  friend struct MemOIRInst;
};

struct ClearInst : public UpdateInst {
public:
  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::CLEAR);
  };

  std::string toString() const override;

protected:
  ClearInst(llvm::CallInst &call_inst) : UpdateInst(call_inst) {}

  friend struct MemOIRInst;
};
// ==============================================================

// Assoc operations
struct HasInst : public AccessInst {
public:
  llvm::Value &getResult() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::HAS);
  };

  std::string toString() const override;

protected:
  HasInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

struct KeysInst : public AccessInst {
public:
  llvm::Value &getResult() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::KEYS);
  };

  std::string toString() const override;

protected:
  KeysInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

// Fold operations.
struct FoldInst : public AccessInst {
public:
  /**
   * Finds the single fold user of this function, if it exists.
   * @param func the LLVM function
   * @returns the single fold, or NULL if none was found.
   */
  static FoldInst *get_single_fold(llvm::Function &func);

  llvm::Value &getResult() const;

  bool isReverse() const;

  llvm::Value &getInitial() const;
  llvm::Use &getInitialAsUse() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  llvm::Function &getBody() const;
  llvm::Value &getBodyOperand() const;
  llvm::Use &getBodyOperandAsUse() const;

  std::optional<ClosedKeyword> getClosed() const;

  using iterator = ClosedKeyword::iterator;
  llvm::iterator_range<iterator> closed();
  iterator closed_begin();
  iterator closed_end();

  using operand_iterator = ClosedKeyword::operand_iterator;
  llvm::iterator_range<operand_iterator> closed_operands();
  operand_iterator closed_ops_begin();
  operand_iterator closed_ops_end();

  using const_iterator = ClosedKeyword::const_iterator;
  llvm::iterator_range<const_iterator> closed() const;
  const_iterator closed_begin() const;
  const_iterator closed_end() const;

  using const_operand_iterator = ClosedKeyword::const_operand_iterator;
  llvm::iterator_range<const_operand_iterator> closed_operands() const;
  const_operand_iterator closed_ops_begin() const;
  const_operand_iterator closed_ops_end() const;

  /**
   * Get the argument corresponding to the accumulator value.
   */
  llvm::Argument &getAccumulatorArgument() const;

  /**
   * Get the argument corresponding to the index value.
   */
  llvm::Argument &getIndexArgument() const;

  /**
   * Get the argument corresponding to the element value, if it exists.
   * NOTE: if the collection being folded over has void element type, this will
   * not exist.
   *
   * @returns the argument, or NULL if it does not exist.
   */
  llvm::Argument *getElementArgument() const;

  /**
   * Get the corresponding argument in the fold function for the given closed
   * use.
   *
   * @param use the use of the closed value
   * @returns the corresponding argument, NULL if the use is not closed on.
   */
  llvm::Argument *getClosedArgument(llvm::Use &use) const;

  /**
   * Fetch the operand use that will be passed to the given argument.
   * @param arg the argument
   * @returns the operand use corresponding to the argument
   */
  llvm::Use *getOperandForArgument(llvm::Argument &arg) const;

  std::string toString() const override;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_FOLD_INST(ENUM, FUNC, CLASS, REVERSE)                           \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  FoldInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  friend struct MemOIRInst;
};

struct ReverseFoldInst : public FoldInst {
public:
  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_FOLD_INST(ENUM, FUNC, CLASS, REVERSE)                           \
  (REVERSE && (I->getKind() == MemOIR_Func::ENUM)) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  ReverseFoldInst(llvm::CallInst &call_inst) : FoldInst(call_inst) {}

  friend struct MemOIRInst;
};

// SSA/readonce operations.
struct UsePHIInst : public MemOIRInst {
public:
  llvm::Value &getResultCollection() const;

  llvm::Value &getUsedCollection() const;
  llvm::Use &getUsedCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::USE_PHI);
  };

  std::string toString() const override;

protected:
  UsePHIInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

struct EndInst : public MemOIRInst {
public:
  llvm::Value &getValue() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::END);
  };

  std::string toString() const override;

protected:
  EndInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

struct ArgPHIInst : public MemOIRInst {
public:
  llvm::Value &getResultCollection() const;

  llvm::Value &getInputCollection() const;
  llvm::Use &getInputCollectionAsUse() const;

  // TODO: add methods for decoding the metadata

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ARG_PHI);
  };

  std::string toString() const override;

protected:
  ArgPHIInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

struct RetPHIInst : public MemOIRInst {
public:
  llvm::Value &getResultCollection() const;

  llvm::Value &getInputCollection() const;
  llvm::Use &getInputCollectionAsUse() const;

  /**
   * @return the function called, or NULL if it was an indirect function
   * invocation.
   */
  llvm::Function *getCalledFunction() const;
  llvm::Value &getCalledOperand() const;
  llvm::Use &getCalledOperandAsUse() const;

  // TODO: add methods for decoding the metadata.

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::RET_PHI);
  };

  std::string toString() const override;

protected:
  RetPHIInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

// Deletion operations
struct DeleteInst : public MemOIRInst {
public:
  llvm::Value &getObject() const;
  llvm::Use &getObjectAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DELETE);
  };

  std::string toString() const override;

protected:
  DeleteInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

// Type checking
struct AssertTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  llvm::Value &getObject() const;
  llvm::Use &getObjectAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSERT_TYPE);
  };

  std::string toString() const override;

protected:
  AssertTypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

struct ReturnTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SET_RETURN_TYPE);
  };

  std::string toString() const override;

protected:
  ReturnTypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  friend struct MemOIRInst;
};

} // namespace llvm::memoir

#endif
