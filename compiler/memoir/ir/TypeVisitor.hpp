#ifndef MEMOIR_TYPEVISITOR_H
#define MEMOIR_TYPEVISITOR_H
#pragma once

#include "memoir/ir/Types.hpp"

namespace llvm::memoir {

#define CHECK_AND_DELEGATE(CLASS_TO_VISIT)                                     \
  else if (isa<CLASS_TO_VISIT>(T)) {                                           \
    DELEGATE(CLASS_TO_VISIT)                                                   \
  }

#define DELEGATE(CLASS_TO_VISIT)                                               \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(T));

template <typename SubClass, typename RetTy = void>
class TypeVisitor {
public:
  // Constructor.
  TypeVisitor() {
    // Do nothing.
  }

  // Visitor methods.
  RetTy visit(Type &T) {
    if (false) {
      // Stub.
    }

    CHECK_AND_DELEGATE(IntegerType)
    CHECK_AND_DELEGATE(FloatType)
    CHECK_AND_DELEGATE(DoubleType)
    CHECK_AND_DELEGATE(PointerType)
    CHECK_AND_DELEGATE(ReferenceType)
    CHECK_AND_DELEGATE(StructType)
    CHECK_AND_DELEGATE(FieldArrayType)
    CHECK_AND_DELEGATE(StaticTensorType)
    CHECK_AND_DELEGATE(TensorType)
    CHECK_AND_DELEGATE(SequenceType)
    CHECK_AND_DELEGATE(AssocArrayType)

    MEMOIR_UNREACHABLE("Could not determine the type of collection!");
  }

  RetTy visitIntegerType(IntegerType &T) {
    DELEGATE(Type);
  };

  RetTy visitFloatType(FloatType &T) {
    DELEGATE(Type);
  };

  RetTy visitDoubleType(DoubleType &T) {
    DELEGATE(Type);
  };

  RetTy visitPointerType(PointerType &T) {
    DELEGATE(Type);
  };

  RetTy visitReferenceType(ReferenceType &T) {
    DELEGATE(Type);
  };

  RetTy visitStructType(StructType &T) {
    DELEGATE(Type);
  };

  RetTy visitCollectionType(CollectionType &T) {
    DELEGATE(Type);
  }

  RetTy visitFieldArrayType(FieldArrayType &T) {
    DELEGATE(CollectionType);
  };

  RetTy visitStaticTensorType(StaticTensorType &T) {
    DELEGATE(CollectionType);
  };

  RetTy visitTensorType(TensorType &T) {
    DELEGATE(CollectionType);
  };

  RetTy visitSequenceType(SequenceType &T) {
    DELEGATE(CollectionType);
  };

  RetTy visitAssocArrayType(AssocArrayType &T) {
    DELEGATE(CollectionType);
  };

  // This class is not cloneable nor assignable.
  TypeVisitor(TypeVisitor &) = delete;
  void operator=(const TypeVisitor &) = delete;
};

} // namespace llvm::memoir

#endif
