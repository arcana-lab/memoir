#include "Normalization.hpp"

using namespace llvm::memoir;

namespace normalization {

void Normalization::transformRuntime() {

  /*
   * Attach metadata to all functions in the program
   */
  for (auto &F : M) {
    MetadataManager::setMetadata(F, MetadataType::INTERNAL);
  }

  return;
}

} // namespace normalization
