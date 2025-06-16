#ifndef FOLIO_SELECTIONMONOMORPHIZATION_H
#define FOLIO_SELECTIONMONOMORPHIZATION_H

namespace folio {

class SelectionMonomorphization {
public:
  SelectionMonomorphization(llvm::Module &M);

protected:
  llvm::Module &M;
};

} // namespace folio

#endif // FOLIO_SELECTIONMONOMORPHIZATION_H
