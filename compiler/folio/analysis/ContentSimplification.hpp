#ifndef FOLIO_ANALYSIS_CONTENTSIMPLIFICATION_H
#define FOLIO_ANALYSIS_CONTENTSIMPLIFICATION_H

#include "folio/analysis/Content.hpp"
#include "folio/analysis/ContentVisitor.hpp"

namespace folio {

class ContentSimplification : ContentVisitor<ContentSimplification, Content &> {
  friend class ContentVisitor<ContentSimplification, Content &>;

public:
  ContentSimplification(Contents &contents) : contents(contents) {}

  Content &simplify(Content &C);

  // Simplification

protected:
  Content *lookup_domain(llvm::Value &V);
  Content *lookup_range(llvm::Value &V);

  Content &visitContent(Content &C);
  Content &visitUnionContent(UnionContent &C);
  Content &visitConditionalContent(ConditionalContent &C);
  Content &visitElementsContent(ElementsContent &C);
  Content &visitKeysContent(KeysContent &C);

  Contents &contents;
};

} // namespace folio

#endif // FOLIO_ANALYSIS_CONTENTSIMPLIFICATION_H
