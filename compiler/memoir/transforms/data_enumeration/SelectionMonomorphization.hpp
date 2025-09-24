#ifndef FOLIO_SELECTIONMONOMORPHIZATION_H
#define FOLIO_SELECTIONMONOMORPHIZATION_H

namespace memoir {

class SelectionMonomorphization {
public:
  SelectionMonomorphization(llvm::Module &M);

protected:
  llvm::Module &M;
};

} // namespace memoir

#endif // FOLIO_SELECTIONMONOMORPHIZATION_H
