#ifndef MEMOIR_COLLECTIONVISITOR_H
#define MEMOIR_COLLECTIONVISITOR_H
#pragma once

#include "memoir/ir/Collections.hpp"

namespace llvm::memoir {

#define DELEGATE(CLASS_TO_VISIT)                                               \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(C));

template <typename SubClass, typename RetTy = void>
class CollectionVisitor {
public:
  // Constructor.
  CollectionVisitor() {
    // Do nothing.
  }

  // Visitor methods.
  RetTy visitCollection(Collection &C) {
    if (isa<BaseCollection>(&C)) {
      DELEGATE(BaseCollection);
    } else if (isa<FieldArray>(&C)) {
      DELEGATE(FieldArray);
    } else if (isa<NestedCollection>(&C)) {
      DELEGATE(NestedCollection);
    } else if (isa<ReferencedCollection>(&C)) {
      DELEGATE(ReferencedCollection);
    } else if (isa<ControlPHICollection>(&C)) {
      DELEGATE(ControlPHICollection);
    } else if (isa<RetPHICollection>(&C)) {
      DELEGATE(RetPHICollection);
    } else if (isa<ArgPHICollection>(&C)) {
      DELEGATE(ArgPHICollection);
    } else if (isa<DefPHICollection>(&C)) {
      DELEGATE(DefPHICollection);
    } else if (isa<UsePHICollection>(&C)) {
      DELEGATE(UsePHICollection);
    } else if (isa<JoinPHICollection>(&C)) {
      DELEGATE(JoinPHICollection);
    } else if (isa<SliceCollection>(&C)) {
      DELEGATE(SliceCollection);
    }

    MEMOIR_UNREACHABLE("Could not determine the type of collection!");
  }

  RetTy visitBaseCollection(BaseCollection &C) {
    DELEGATE(Collection);
  };

  RetTy visitFieldArray(FieldArray &C) {
    DELEGATE(FieldArray);
  };

  RetTy visitNestedCollection(NestedCollection &C) {
    DELEGATE(NestedCollection);
  };

  RetTy visitReferencedCollection(ReferencedCollection &C) {
    DELEGATE(ReferencedCollection);
  };

  RetTy visitControlPHICollection(ControlPHICollection &C) {
    DELEGATE(ControlPHICollection);
  };

  RetTy visitRetPHICollection(RetPHICollection &C) {
    DELEGATE(RetPHICollection);
  };

  RetTy visitArgPHICollection(ArgPHICollection &C) {
    DELEGATE(ArgPHICollection);
  };

  RetTy visitDefPHICollection(DefPHICollection &C) {
    DELEGATE(DefPHICollection);
  };

  RetTy visitUsePHICollection(UsePHICollection &C) {
    DELEGATE(UsePHICollection);
  };

  RetTy visitJoinPHICollection(JoinPHICollection &C) {
    DELEGATE(JoinPHICollection);
  };

  RetTy visitSliceCollection(SliceCollection &C) {
    DELEGATE(SliceCollection);
  };

  // This class is not cloneable nor assignable.
  CollectionVisitor(CollectionVisitor &) = delete;
  void operator=(const CollectionVisitor &) = delete;
};

} // namespace llvm::memoir

#endif
