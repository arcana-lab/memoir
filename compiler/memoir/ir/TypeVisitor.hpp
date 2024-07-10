#ifndef MEMOIR_TYPEVISITOR_H
#define MEMOIR_TYPEVISITOR_H

#include "memoir/ir/Types.hpp"

namespace llvm::memoir {

#define CHECK_AND_DELEGATE_TYPE(CLASS_TO_VISIT)                                \
  else if (isa<CLASS_TO_VISIT>(T)) {                                           \
    DELEGATE_TYPE(CLASS_TO_VISIT)                                              \
  }

#define DELEGATE_TYPE(CLASS_TO_VISIT)                                          \
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

    CHECK_AND_DELEGATE_TYPE(IntegerType)
    CHECK_AND_DELEGATE_TYPE(FloatType)
    CHECK_AND_DELEGATE_TYPE(DoubleType)
    CHECK_AND_DELEGATE_TYPE(PointerType)
    CHECK_AND_DELEGATE_TYPE(VoidType)
    CHECK_AND_DELEGATE_TYPE(ReferenceType)
    CHECK_AND_DELEGATE_TYPE(StructType)
    CHECK_AND_DELEGATE_TYPE(FieldArrayType)
    CHECK_AND_DELEGATE_TYPE(StaticTensorType)
    CHECK_AND_DELEGATE_TYPE(TensorType)
    CHECK_AND_DELEGATE_TYPE(SequenceType)
    CHECK_AND_DELEGATE_TYPE(AssocArrayType)

    MEMOIR_UNREACHABLE("Could not determine the type of collection!");
  }

  RetTy visitIntegerType(IntegerType &T) {
    DELEGATE_TYPE(Type);
  };

  RetTy visitFloatType(FloatType &T) {
    DELEGATE_TYPE(Type);
  };

  RetTy visitDoubleType(DoubleType &T) {
    DELEGATE_TYPE(Type);
  };

  RetTy visitPointerType(PointerType &T) {
    DELEGATE_TYPE(Type);
  };

  RetTy visitVoidType(VoidType &T) {
    DELEGATE_TYPE(Type);
  };

  RetTy visitReferenceType(ReferenceType &T) {
    DELEGATE_TYPE(Type);
  };

  RetTy visitStructType(StructType &T) {
    DELEGATE_TYPE(Type);
  };

  RetTy visitCollectionType(CollectionType &T) {
    DELEGATE_TYPE(Type);
  }

  RetTy visitFieldArrayType(FieldArrayType &T) {
    DELEGATE_TYPE(CollectionType);
  };

  RetTy visitStaticTensorType(StaticTensorType &T) {
    DELEGATE_TYPE(CollectionType);
  };

  RetTy visitTensorType(TensorType &T) {
    DELEGATE_TYPE(CollectionType);
  };

  RetTy visitSequenceType(SequenceType &T) {
    DELEGATE_TYPE(CollectionType);
  };

  RetTy visitAssocArrayType(AssocArrayType &T) {
    DELEGATE_TYPE(CollectionType);
  };

  // This class is not cloneable nor assignable.
  TypeVisitor(TypeVisitor &) = delete;
  void operator=(const TypeVisitor &) = delete;
};

} // namespace llvm::memoir

#endif
