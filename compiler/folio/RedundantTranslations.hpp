#ifndef FOLIO_REDUNDANTTRANSLATIONS_H
#define FOLIO_REDUNDANTTRANSLATIONS_H

#include "llvm/IR/Use.h"

#include "folio/Utilities.hpp"

namespace folio {

void eliminate_redundant_translations(LocalMap<Set<llvm::Use *>> &to_decode,
                                      LocalMap<Set<llvm::Use *>> &to_encode,
                                      LocalMap<Set<llvm::Use *>> &to_addkey);

} // namespace folio

#endif // FOLIO_REDUNDANTTRANSLATIONS_H
