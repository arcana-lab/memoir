#ifndef FOLIO_REDUNDANTTRANSLATIONS_H
#define FOLIO_REDUNDANTTRANSLATIONS_H

#include "llvm/IR/Use.h"

#include "Utilities.hpp"

namespace memoir {

void eliminate_redundant_translations(
    const Map<llvm::Function *, Set<llvm::Value *>> &encoded,
    Map<llvm::Function *, Set<llvm::Use *>> &to_decode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_encode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_addkey);

} // namespace memoir

#endif // FOLIO_REDUNDANTTRANSLATIONS_H
