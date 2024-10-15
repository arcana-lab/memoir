#ifndef FOLIO_ANALYSIS_CONTENTSIMPLIFICATION_H
#define FOLIO_ANALYSIS_CONTENTSIMPLIFICATION_H

#include "folio/analysis/Content.hpp"
#include "folio/analysis/ContentVisitor.hpp"

namespace folio {

class ContentSimplification : ContentVisitor<ContentSimplification, Content &> {
  friend class ContentVisitor<ContentSimplification, Content &>;

public:
  ContentSimplification(Contents &contents) : contents(&contents) {}
  ContentSimplification() : contents(nullptr) {}

  Content &simplify(Content &C);

  static bool is_simple(Content &C);

protected:
  // Borrowed State.
  Contents *contents;

  // Helper methods.
  Content *lookup_domain(llvm::Value &V);
  Content *lookup_range(llvm::Value &V);

  Content &visitContent(Content &C);
  Content &visitUnionContent(UnionContent &C);
  Content &visitFieldContent(FieldContent &C);
  Content &visitConditionalContent(ConditionalContent &C);
  Content &visitElementContent(ElementContent &C);
  Content &visitElementsContent(ElementsContent &C);
  Content &visitKeyContent(KeyContent &C);
  Content &visitKeysContent(KeysContent &C);
};

Content &simplify(Content &C);

} // namespace folio

#endif // FOLIO_ANALYSIS_CONTENTSIMPLIFICATION_H
