#include "common/utility/Metadata.hpp"

namespace llvm {
namespace memoir {

void MetadataManager::setMetadata(Function &F, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.setMetadata(F, mdKind);

  return;
}

bool MetadataManager::hasMetadata(Function &F, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  return MM.hasMetadata(F, mdKind);
}

void MetadataManager::setMetadata(Instruction &I, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.setMetadata(I, mdKind);

  return;
}

bool MetadataManager::hasMetadata(Instruction &I, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  return MM.hasMetadata(I, mdKind);
}

/*
 * Private and internal methods
 */

void MetadataManager::setMetadata(Function &F, std::string kind) {
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

bool MetadataManager::hasMetadata(Function &F, std::string kind) {
  return (F.getMetadata(kind) != nullptr);
}

void MetadataManager::setMetadata(Instruction &I, std::string kind) {
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

bool MetadataManager::hasMetadata(Instruction &I, std::string kind) {
  return (I.getMetadata(kind) != nullptr);
}

} // namespace memoir
} // namespace llvm
