#include "Normalization.hpp"

using namespace llvm::memoir;

namespace normalization {

void Normalization::transformRuntime() {

  /*
   * Get the metadata manager
   */
  auto &MM = MetadataManager::getManager();

  /*
   * Attach metadata to all functions in the program
   */
  for (auto &F : M) {
    MM.setMetadata(F, MetadataType::INTERNAL);
  }

  return;
}

} // namespace normalization
