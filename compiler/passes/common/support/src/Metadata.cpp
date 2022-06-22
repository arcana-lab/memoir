#include "common/support/Metadata.hpp"

namespace llvm {
namespace memoir {

void MetadataManager::setMetadata(Function &F, MetadataType MT) {

  auto mdKind = this->MDtoString[MT];

  this->setMetadata(F, mdKind);

  return;
}

bool MetadataManager::hasMetadata(Function &F, MetadataType MT) {

  auto mdKind = this->MDtoString[MT];

  return this->hasMetadata(F, mdKind);
}

void MetadataManager::setMetadata(Instruction &I, MetadataType MT) {
  auto mdKind = this->MDtoString[MT];

  this->setMetadata(I, mdKind);

  return;
}

bool MetadataManager::hasMetadata(Instruction &I, MetadataType MT) {
  auto mdKind = this->MDtoString[MT];

  return this->hasMetadata(I, mdKind);
}

/*
 * Private and internal methods
 */

void MetadataManager::setMetadata(Function &F, StringRef kind) {
  /*
   * Create the metadata
   */
  auto &Context = F.getContext();
  auto mdString = MDString::get(Context, kind);
  auto mdNode = MDNode::get(Context, ArrayRef<Metadata *>({ mdString }));

  /*
   * Attach the metadata to the function
   */
  F.setMetadata(kind, mdNode);

  return;
}

bool MetadataManager::hasMetadata(Function &F, StringRef kind) {
  return (F.getMetadata(kind) != nullptr);
}

void MetadataManager::setMetadata(Instruction &I, StringRef kind) {
  /*
   * Create the metadata
   */
  auto &Context = I.getContext();
  auto mdString = MDString::get(Context, kind);
  auto mdNode = MDNode::get(Context, ArrayRef<Metadata *>({ mdString }));

  /*
   * Attach the metadata to the function
   */
  I.setMetadata(kind, mdNode);

  return;
}

bool MetadataManager::hasMetadata(Instruction &I, StringRef kind) {
  return (I.getMetadata(kind) != nullptr);
}

} // namespace memoir
} // namespace llvm
