#ifndef FOLIO_REDUNDANTTRANSLATIONS_H
#define FOLIO_REDUNDANTTRANSLATIONS_H

#include "llvm/IR/Use.h"

#include "folio/Utilities.hpp"

namespace folio {

void eliminate_redundant_translations(
    const Map<llvm::Function *, Set<llvm::Value *>> &encoded,
    Map<llvm::Function *, Set<llvm::Use *>> &to_decode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_encode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_addkey);

} // namespace folio

#endif // FOLIO_REDUNDANTTRANSLATIONS_H
