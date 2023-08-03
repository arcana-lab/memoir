#include "memoir/utility/Metadata.hpp"

namespace llvm {
namespace memoir {

void MetadataManager::setMetadata(llvm::Function &F, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.setMetadata(F, mdKind);

  return;
}

void MetadataManager::setMetadata(llvm::Function &F,
                                  MetadataType MT,
                                  llvm::Value *value) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.setMetadata(F, mdKind, value);

  return;
}

bool MetadataManager::hasMetadata(llvm::Function &F, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  return MM.hasMetadata(F, mdKind);
}

llvm::Value *MetadataManager::getMetadata(llvm::Function &F, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  return MM.getMetadata(F, mdKind);
}

void MetadataManager::setMetadata(llvm::Instruction &I, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.setMetadata(I, mdKind);

  return;
}

void MetadataManager::setMetadata(llvm::Instruction &I,
                                  MetadataType MT,
                                  llvm::Value *value) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.setMetadata(I, mdKind, value);

  return;
}

bool MetadataManager::hasMetadata(llvm::Instruction &I, MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  return MM.hasMetadata(I, mdKind);
}

llvm::Value *MetadataManager::getMetadata(llvm::Instruction &I,
                                          MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  return MM.getMetadata(I, mdKind);
}

/*
 * Private and internal methods
 */
void MetadataManager::setMetadata(llvm::Function &F, std::string kind) {
  /*
   * Create the metadata
   */
  auto &Context = F.getContext();
  auto mdString = llvm::MDString::get(Context, kind);
  auto mdNode =
      llvm::MDNode::get(Context, ArrayRef<llvm::Metadata *>({ mdString }));

  /*
   * Attach the metadata to the function
   */
  F.setMetadata(kind, mdNode);

  return;
}

void MetadataManager::setMetadata(llvm::Function &F,
                                  std::string kind,
                                  llvm::Value *value) {
  /*
   * Create the metadata
   */
  auto &Context = F.getContext();
  auto *mdString = llvm::MDString::get(Context, kind);
  auto *mdValue = llvm::ValueAsMetadata::get(value);
  auto *mdNode =
      llvm::MDNode::get(Context,
                        ArrayRef<llvm::Metadata *>({ mdString, mdValue }));

  /*
   * Attach the metadata to the function
   */
  F.setMetadata(kind, mdNode);

  return;
}

bool MetadataManager::hasMetadata(llvm::Function &F, std::string kind) {
  return (F.getMetadata(kind) != nullptr);
}

llvm::Value *MetadataManager::getMetadata(llvm::Function &F, std::string kind) {

  // Find the metadata of the given kind.
  auto *mdNode = F.getMetadata(kind);
  if (mdNode == nullptr) {
    return nullptr;
  }

  // Get the ValueAsMetadata from the tuple.
  if (mdNode->getNumOperands() < 2) {
    return nullptr;
  }
  auto *mdValue = mdNode->getOperand(1).get();
  if (mdValue == nullptr) {
    return nullptr;
  }

  // Unpack the ValueAsMetadata.
  auto mdValueAsMetadata = dyn_cast<llvm::ValueAsMetadata>(mdValue);
  if (!mdValueAsMetadata) {
    return nullptr;
  }
  auto *value = mdValueAsMetadata->getValue();

  // Return it.
  return value;
}

void MetadataManager::setMetadata(llvm::Instruction &I, std::string kind) {
  /*
   * Create the metadata
   */
  auto &Context = I.getContext();
  auto mdString = llvm::MDString::get(Context, kind);
  auto mdNode =
      llvm::MDNode::get(Context, ArrayRef<llvm::Metadata *>({ mdString }));

  /*
   * Attach the metadata to the function
   */
  I.setMetadata(kind, mdNode);

  return;
}

void MetadataManager::setMetadata(llvm::Instruction &I,
                                  std::string kind,
                                  llvm::Value *value) {
  // Create the metadata
  auto &Context = I.getContext();
  auto *mdString = llvm::MDString::get(Context, kind);
  auto *mdValue = llvm::ValueAsMetadata::get(value);
  auto *mdNode =
      llvm::MDNode::get(Context,
                        ArrayRef<llvm::Metadata *>({ mdString, mdValue }));

  // Attach the metadata to the function
  I.setMetadata(kind, mdNode);

  return;
}

bool MetadataManager::hasMetadata(llvm::Instruction &I, std::string kind) {
  return (I.getMetadata(kind) != nullptr);
}

llvm::Value *MetadataManager::getMetadata(llvm::Instruction &I,
                                          std::string kind) {
  // Find the metadata of the given kind.
  auto *mdNode = I.getMetadata(kind);
  if (mdNode == nullptr) {
    return nullptr;
  }

  // Get the ValueAsMetadata from the tuple.
  if (mdNode->getNumOperands() < 2) {
    return nullptr;
  }
  auto *mdValue = mdNode->getOperand(1).get();
  if (mdValue == nullptr) {
    return nullptr;
  }

  // Unpack the ValueAsMetadata.
  auto mdValueAsMetadata = dyn_cast<llvm::ValueAsMetadata>(mdValue);
  if (!mdValueAsMetadata) {
    return nullptr;
  }
  auto *value = mdValueAsMetadata->getValue();

  // Return it.
  return value;
}

} // namespace memoir
} // namespace llvm
