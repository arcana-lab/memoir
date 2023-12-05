#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Assert.hpp"

namespace llvm {
namespace memoir {

// Static entry points.
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

void MetadataManager::insertMetadata(llvm::Function &F,
                                     MetadataType MT,
                                     std::string str) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.insertMetadata(F, mdKind, str);

  return;
}

void MetadataManager::removeMetadata(llvm::Function &F,
                                     MetadataType MT,
                                     std::string str) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.removeMetadata(F, mdKind, str);

  return;
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

void MetadataManager::insertMetadata(llvm::Instruction &I,
                                     MetadataType MT,
                                     std::string str) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.insertMetadata(I, mdKind, str);

  return;
}

void MetadataManager::removeMetadata(llvm::Instruction &I,
                                     MetadataType MT,
                                     std::string str) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  MM.removeMetadata(I, mdKind, str);

  return;
}

llvm::Value *MetadataManager::getMetadata(llvm::Instruction &I,
                                          MetadataType MT) {
  auto &MM = MetadataManager::getManager();

  auto mdKind = MM.MDtoString[MT];

  return MM.getMetadata(I, mdKind);
}

// Private and internal methods

//// Function metadata.
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

void MetadataManager::insertMetadata(llvm::Function &F,
                                     std::string kind,
                                     std::string value) {
  // Get the context.
  auto &Context = F.getContext();

  // Get the current metadata node.
  auto *current_md = F.getMetadata(kind);

  // If there isn't a metadata there yet, create it.
  if (current_md == nullptr) {
    auto *md_string = llvm::MDString::get(Context, value);
    auto *md_tuple =
        llvm::MDTuple::getDistinct(Context,
                                   ArrayRef<llvm::Metadata *>(md_string));
    F.setMetadata(kind, md_tuple);
    return;
  }

  // Otherwise, search the tuple for the key we are inserting.
  auto &current_md_tuple =
      MEMOIR_SANITIZE(dyn_cast<llvm::MDTuple>(current_md),
                      "Trying the insert metadata to a non-tuple Metadata");
  for (auto &operand : current_md_tuple.operands()) {
    auto *operand_md = operand.get();
    auto *operand_md_string = dyn_cast_or_null<llvm::MDString>(operand_md);
    MEMOIR_NULL_CHECK(operand_md_string, "MDTuple contains non-MDString");
    if (operand_md_string->getString().str() == value) {
      return;
    }
  }

  // If we didn't find the key, create and push the metadata onto the tuple.
  auto *md_string = llvm::MDString::get(Context, value);
  vector<llvm::Metadata *> new_operands(current_md_tuple.op_begin(),
                                        current_md_tuple.op_end());
  new_operands.push_back(md_string);
  auto *new_md_tuple = llvm::MDTuple::getDistinct(Context, new_operands);
  F.setMetadata(kind, new_md_tuple);

  return;
}

void MetadataManager::removeMetadata(llvm::Function &F,
                                     std::string kind,
                                     std::string value) {
  // Get the context.
  auto &Context = F.getContext();

  // Get the current metadata node.
  auto *current_md = F.getMetadata(kind);

  // If there isn't a metadata there yet, we're done.
  if (current_md == nullptr) {
    return;
  }

  // Otherwise, search the tuple for the key we are inserting.
  auto &current_md_tuple =
      MEMOIR_SANITIZE(dyn_cast<llvm::MDTuple>(current_md),
                      "Trying to remove metadata from a non-tuple Metadata");
  bool removed = true;
  std::vector<llvm::Metadata *> new_operands = {};
  for (auto &operand : current_md_tuple.operands()) {
    auto *operand_md = operand.get();
    auto *operand_md_string = dyn_cast_or_null<llvm::MDString>(operand_md);
    MEMOIR_NULL_CHECK(operand_md_string, "MDTuple contains non-MDString");
    if (operand_md_string->getString().str() != value) {
      new_operands.push_back(operand_md_string);
    } else {
      removed = true;
    }
  }

  // If we removed something, change the metadata attached to the string.
  if (removed) {
    auto *md_tuple = llvm::MDTuple::getDistinct(
        Context,
        llvm::ArrayRef<llvm::Metadata *>(new_operands.data(),
                                         new_operands.size()));
    F.setMetadata(kind, md_tuple);
  }

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

//// Instruction metadata.
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

void MetadataManager::insertMetadata(llvm::Instruction &I,
                                     std::string kind,
                                     std::string value) {
  // Get the context.
  auto &Context = I.getContext();

  // Get the current metadata node.
  auto *current_md = I.getMetadata(kind);

  // If there isn't a metadata there yet, create it.
  if (current_md == nullptr) {
    auto *md_string = llvm::MDString::get(Context, value);
    auto *md_tuple =
        llvm::MDTuple::getDistinct(Context,
                                   ArrayRef<llvm::Metadata *>(md_string));
    I.setMetadata(kind, md_tuple);
    return;
  }

  // Otherwise, search the tuple for the key we are inserting.
  auto &current_md_tuple =
      MEMOIR_SANITIZE(dyn_cast<llvm::MDTuple>(current_md),
                      "Trying to insert metadata to a non-tuple Metadata");
  for (auto &operand : current_md_tuple.operands()) {
    auto *operand_md = operand.get();
    auto *operand_md_string = dyn_cast_or_null<llvm::MDString>(operand_md);
    MEMOIR_NULL_CHECK(operand_md_string, "MDTuple contains non-MDString");
    if (operand_md_string->getString().str() == value) {
      return;
    }
  }

  // If we didn't find the key, create and push the metadata onto the tuple.
  auto *md_string = llvm::MDString::get(Context, value);
  vector<llvm::Metadata *> new_operands(current_md_tuple.op_begin(),
                                        current_md_tuple.op_end());
  new_operands.push_back(md_string);
  auto *new_md_tuple = llvm::MDTuple::getDistinct(Context, new_operands);
  I.setMetadata(kind, new_md_tuple);

  return;
}

void MetadataManager::removeMetadata(llvm::Instruction &I,
                                     std::string kind,
                                     std::string value) {
  // Get the context.
  auto &Context = I.getContext();

  // Get the current metadata node.
  auto *current_md = I.getMetadata(kind);

  // If there isn't a metadata there yet, we're done.
  if (current_md == nullptr) {
    return;
  }

  // Otherwise, search the tuple for the key we are inserting.
  auto &current_md_tuple =
      MEMOIR_SANITIZE(dyn_cast<llvm::MDTuple>(current_md),
                      "Trying to remove metadata from a non-tuple Metadata");
  bool removed = true;
  std::vector<llvm::Metadata *> new_operands = {};
  for (auto &operand : current_md_tuple.operands()) {
    auto *operand_md = operand.get();
    auto *operand_md_string = dyn_cast_or_null<llvm::MDString>(operand_md);
    MEMOIR_NULL_CHECK(operand_md_string, "MDTuple contains non-MDString");
    if (operand_md_string->getString().str() != value) {
      new_operands.push_back(operand_md_string);
    } else {
      removed = true;
    }
  }

  // If we removed something, change the metadata attached to the string.
  if (removed) {
    auto *md_tuple = llvm::MDTuple::getDistinct(
        Context,
        llvm::ArrayRef<llvm::Metadata *>(new_operands.data(),
                                         new_operands.size()));
    I.setMetadata(kind, md_tuple);
  }

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
