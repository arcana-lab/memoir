#include "Normalization.hpp"

namespace normalization {

void Normalization::transformRuntime() {

  /*
   * Attach metadata to all functions in the program
   */
  for (auto &F : M) {
    setRuntimeMetadata(&F);
  }

  return;
}

void Normalization::setRuntimeMetadata(Function *F) {

  /*
   * Create the metadata
   */
  auto &Context = F->getContext();
  auto mdString = MDString::get(Context, OBJECTIR_INTERNAL);
  auto mdNode = MDNode::get(Context, ArrayRef<Metadata *>({ mdString }));

  /*
   * Attach the metadata to the function
   */
  F->addMetadata(OBJECTIR_INTERNAL, *mdNode);

  return;
}

} // namespace normalization
