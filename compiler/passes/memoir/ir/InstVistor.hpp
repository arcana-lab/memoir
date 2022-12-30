#ifndef COMMON_INST_VISITOR_H
#define COMMON_INST_VISITOR_H
#pragma once

namespace llvm::memoir {

#define DELEGATE(CLASS_TO_VISIT)                                               \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I));

template <typename SubClass, typename RetTy = void>
struct InstVisitor : llvm::InstVisitor<llvm::memoir::InstVisitor, RetTy> {
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
    return DELEGATE(CLASS);
#include "memoir/ir/Instructions.def"
    }
  };

  RetTy visitLLVMCallInst(llvm::CallInst &I) {
    DELEGATE(Instruction);
  };
  RetTy visitMemOIRInst(MemOIRInst &I) {
    DELEGATE(Instruction);
  };
  RetTy visitAccessInst(AccessInst &I) {
    DELEGATE(Instruction);
  };
  RetTy visitReadInst(ReadInst &I) {
    DELEGATE(AccessInst);
  };
  RetTy visitWriteInst(WriteInst &I) {
    DELEGATE(AccessInst);
  };
  RetTy visitGetInst(GetInst &I) {
    DELEGATE(AccessInst);
  };
  RetTy visitTypeInst(TypeInst &I) {
    DELEGATE(Instruction);
  };
#define HANDLE_INST(ENUM, FUNC, CLASS) // Stubs for base classes
  RetTy visit##CLASS(CLASS##Inst &I);
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS##Inst(CLASS##Inst &I) {                                   \
    DELEGATE(TypeInst);                                                        \
  };
#define HANDLE_ALLOC_INST(ENUM, FUNC, CLASS)                                   \
  RetTy visit##CLASS(CLASS##Inst &I) {                                         \
    DELEGATE(AllocInst);                                                       \
  };
#define HANDLE_READ_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS##Inst &I) {                                         \
    DELEGATE(ReadInst);                                                        \
  };
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS)                                   \
  RetTy visit##CLASS(CLASS##Inst &I) {                                         \
    DELEGATE(WriteInst);                                                       \
  };
#define HANDLE_GET_INST(ENUM, FUNC, CLASS)                                     \
  RetTy visit##CLASS(CLASS##Inst &I) {                                         \
    DELEGATE(GetInst);                                                         \
  };
#include "memoir/ir/Instructions.def"

protected:
};

} // namespace llvm::memoir

#endif
