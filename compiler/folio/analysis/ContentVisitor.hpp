#ifndef FOLIO_ANALYSIS_CONTENTVISITOR_H
#define FOLIO_ANALYSIS_CONTENTVISITOR_H

#include "memoir/support/Casting.hpp"

#include "folio/analysis/Content.hpp"

namespace folio {

#define CHECK_AND_DELEGATE_CONTENT(CLASS_TO_VISIT)                             \
  else if (llvm::memoir::isa<CLASS_TO_VISIT>(T)) {                             \
    DELEGATE_CONTENT(CLASS_TO_VISIT)                                           \
  }

#define DELEGATE_CONTENT(CLASS_TO_VISIT)                                       \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(T));

template <typename SubClass, typename RetTy = void>
class ContentVisitor {
public:
  // Constructor.
  ContentVisitor() {
    // Do nothing.
  }

  // Visitor methods.
  RetTy visit(Content &T) {
    if (false) {
      // Stub.
    }

    CHECK_AND_DELEGATE_CONTENT(UnderdefinedContent)
    CHECK_AND_DELEGATE_CONTENT(EmptyContent)
    CHECK_AND_DELEGATE_CONTENT(StructContent)
    CHECK_AND_DELEGATE_CONTENT(ScalarContent)
    CHECK_AND_DELEGATE_CONTENT(KeysContent)
    CHECK_AND_DELEGATE_CONTENT(RangeContent)
    CHECK_AND_DELEGATE_CONTENT(ElementsContent)
    CHECK_AND_DELEGATE_CONTENT(FieldContent)
    CHECK_AND_DELEGATE_CONTENT(SubsetContent)
    CHECK_AND_DELEGATE_CONTENT(ConditionalContent)
    CHECK_AND_DELEGATE_CONTENT(TupleContent)
    CHECK_AND_DELEGATE_CONTENT(UnionContent)

    MEMOIR_UNREACHABLE("Could not determine the content of collection!");
  }

  RetTy visitUnderdefinedContent(UnderdefinedContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitEmptyContent(EmptyContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitScalarContent(ScalarContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitKeysContent(KeysContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitRangeContent(RangeContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitStructContent(StructContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitElementsContent(ElementsContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitFieldContent(FieldContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitSubsetContent(SubsetContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitConditionalContent(ConditionalContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitTupleContent(TupleContent &T) {
    DELEGATE_CONTENT(Content);
  };

  RetTy visitUnionContent(UnionContent &T) {
    DELEGATE_CONTENT(Content);
  };

  // This class is not cloneable nor assignable.
  ContentVisitor(ContentVisitor &) = delete;
  void operator=(const ContentVisitor &) = delete;
};

} // namespace folio

#endif // FOLIO_ANALYSIS_CONTENTVISITOR_H
