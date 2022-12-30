#ifndef COMMON_INST_VISITOR_H
#define COMMON_INST_VISITOR_H
#pragma once

namespace llvm::memoir {

#define DELEGATE(CLASS_TO_VISIT)                                               \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(CLASS_TO_VISIT(I))

#define DELEGATE_MEMOIR(CLASS_TO_VISIT)                                        \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I))

#define DELEGATE_LLVM(CLASS_TO_VISIT)                                          \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I.getCallInst()))

template <typename SubClass, typename RetTy = void>
struct InstVisitor : llvm::InstVisitor<SubClass, RetTy> {
public:
  RetTy visitCallInst(llvm::CallInst &I) {
    static_assert(std::is_base_of<InstVisitor, SubClass>::value,
                  "Must pass the derived type to this template!");

    if (!FunctionNames::is_memoir_call(I)) {
      return static_cast<SubClass *>(this)->visitLLVMCallInst(I);
    }

    auto memoir_enum = FunctionNames::get_memoir_enum(I);
    switch (memoir_enum) {
      default:
        MEMOIR_UNREACHABLE("Unknown MemOIR instruction encountered!");
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM:                                                      \
    DELEGATE(CLASS);
#include "memoir/ir/Instructions.def"
    }
  };

  RetTy visitLLVMCallInst(llvm::CallInst &I) {
    return static_cast<SubClass *>(this)->visitInstruction(
        static_cast<Instruction &>(I));
  };
  RetTy visitMemOIRInst(MemOIRInst &I) {
    DELEGATE_LLVM(Instruction);
  };
  RetTy visitAccessInst(AccessInst &I) {
    DELEGATE_LLVM(Instruction);
  };
  RetTy visitReadInst(ReadInst &I) {
    DELEGATE_MEMOIR(AccessInst);
  };
  RetTy visitStructReadInst(StructReadInst &I) {
    DELEGATE_MEMOIR(ReadInst);
  };
  RetTy visitIndexReadInst(IndexReadInst &I) {
    DELEGATE_MEMOIR(ReadInst);
  };
  RetTy visitAssocReadInst(AssocReadInst &I) {
    DELEGATE_MEMOIR(ReadInst);
  };
  RetTy visitWriteInst(WriteInst &I) {
    DELEGATE_MEMOIR(AccessInst);
  };
  RetTy visitStructWriteInst(StructWriteInst &I) {
    DELEGATE_MEMOIR(WriteInst);
  };
  RetTy visitIndexWriteInst(IndexWriteInst &I) {
    DELEGATE_MEMOIR(WriteInst);
  };
  RetTy visitAssocWriteInst(AssocWriteInst &I) {
    DELEGATE_MEMOIR(WriteInst);
  };
  RetTy visitGetInst(GetInst &I) {
    DELEGATE_MEMOIR(AccessInst);
  };
  RetTy visitStructGetInst(StructGetInst &I) {
    DELEGATE_MEMOIR(GetInst);
  };
  RetTy visitIndexGetInst(IndexGetInst &I) {
    DELEGATE_MEMOIR(GetInst);
  };
  RetTy visitAssocGetInst(AssocGetInst &I) {
    DELEGATE_MEMOIR(GetInst);
  };
  RetTy visitTypeInst(TypeInst &I) {
    DELEGATE_LLVM(Instruction);
  };
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_LLVM(Instruction);                                                \
  };
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_MEMOIR(TypeInst);                                                 \
  };
#define HANDLE_ALLOC_INST(ENUM, FUNC, CLASS)                                   \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_MEMOIR(AllocInst);                                                \
  };
#define HANDLE_ACCESS_INST(ENUM, FUNC, CLASS) /* No handling. */
#include "memoir/ir/Instructions.def"

protected:
};

} // namespace llvm::memoir

#endif
