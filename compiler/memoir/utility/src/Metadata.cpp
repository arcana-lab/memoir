#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

namespace detail {

std::string get_metadata_tag(MetadataKind kind) {
  switch (kind) {
    case MetadataKind::MD_STRUCT_FIELDS:
      return "memoir.fields";
#define METADATA(ENUM, STR, CLASS)                                             \
  case MetadataKind::ENUM:                                                     \
    return STR;
#include "memoir/utility/Metadata.def"
    default:
      return "";
  }
}

llvm::MDNode *find_tagged_metadata(llvm::MDTuple &tuple, std::string tag) {
  for (auto &operand : tuple.operands()) {
    auto *op_metadata = dyn_cast_or_null<llvm::MDNode>(operand.get());
    if (not op_metadata) {
      continue;
    }

    auto *op_tag =
        dyn_cast_or_null<llvm::MDString>(op_metadata->getOperand(0).get());
    if (op_tag->getString().str() == tag) {
      return op_metadata;
    }
  }

  return nullptr;
}

llvm::MDTuple *get_field_metadata(StructType &type, unsigned field) {
  // Get the fields metadata.
  auto &I = type.getDefinition();
  auto tag = get_metadata_tag(MetadataKind::MD_STRUCT_FIELDS);
  auto *fields_metadata =
      dyn_cast_or_null<llvm::MDTuple>(I.getCallInst().getMetadata(tag));
  if (not fields_metadata) {
    return nullptr;
  }

  // Find the corresponding field metadata.
  for (auto &operand : fields_metadata->operands()) {
    auto *op_node = dyn_cast_or_null<llvm::MDNode>(operand.get());
    auto *first = op_node->op_begin()->get();
    if (auto *first_as_constant =
            dyn_cast_or_null<llvm::ConstantAsMetadata>(first)) {
      auto *constant = first_as_constant->getValue();
      auto *constant_int = dyn_cast<llvm::ConstantInt>(constant);
      auto constant_value = constant_int->getZExtValue();
      if (constant_value == field) {
        auto *second = op_node->getOperand(1).get();
        return dyn_cast_or_null<llvm::MDTuple>(second);
      }
    }
  }

  // Could not find any metadata for the corresponding field.
  return nullptr;
}

llvm::MDTuple &get_or_add_field_metadata(StructType &type, unsigned field) {
  // Try to get the metadata.
  if (auto *found = get_field_metadata(type, field)) {
    return *found;
  }

  // Construct the fields metadata.
  auto &I = type.getDefinition();
  auto &context = I.getCallInst().getContext();
  auto tag = get_metadata_tag(MetadataKind::MD_STRUCT_FIELDS);
  auto *fields_metadata =
      dyn_cast_or_null<llvm::MDTuple>(I.getCallInst().getMetadata(tag));
  if (not fields_metadata) {
    fields_metadata =
        llvm::MDTuple::getDistinct(context,
                                   llvm::ArrayRef<llvm::Metadata *>({}));
  }

  // Create the tuple for the given field.
  auto &tuple = MEMOIR_SANITIZE(
      llvm::MDTuple::getDistinct(context, llvm::ArrayRef<llvm::Metadata *>({})),
      "Failed to create MDTuple");

  // Create the field index constant as metadata.
  auto *field_integer_type = llvm::IntegerType::get(context, 8);
  auto *field_constant = llvm::ConstantInt::get(field_integer_type, field);
  auto *field_constant_as_metadata =
      llvm::ConstantAsMetadata::get(field_constant);

  // Create the node for the given field.
  auto &field_metadata = MEMOIR_SANITIZE(
      llvm::MDNode::getDistinct(context,
                                llvm::ArrayRef<llvm::Metadata *>(
                                    { field_constant_as_metadata, &tuple })),
      "Failed to create MDNode");

  // Append the field.
  fields_metadata->push_back(&field_metadata);

  // Return the inner tuple.
  return tuple;
}

} // namespace detail

#define METADATA(ENUM, STR, CLASS)                                             \
  template <>                                                                  \
  std::optional<CLASS> Metadata::get(llvm::Function &F) {                      \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    if (not F.hasMetadata(tag)) {                                              \
      return std::nullopt;                                                     \
    }                                                                          \
    auto *metadata = dyn_cast_or_null<llvm::MDTuple>(F.getMetadata(tag));      \
    return (metadata == nullptr) ? std::nullopt                                \
                                 : std::make_optional(CLASS(*metadata));       \
  }                                                                            \
  template <>                                                                  \
  CLASS Metadata::get_or_add<CLASS>(llvm::Function & F) {                      \
    auto got = Metadata::get<CLASS>(F);                                        \
    if (got.has_value()) {                                                     \
      return got.value();                                                      \
    }                                                                          \
    auto &context = F.getContext();                                            \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    auto *metadata =                                                           \
        llvm::MDTuple::getDistinct(context,                                    \
                                   llvm::ArrayRef<llvm::Metadata *>({}));      \
    F.setMetadata(tag, metadata);                                              \
    return CLASS(*metadata);                                                   \
  }                                                                            \
  template <>                                                                  \
  bool Metadata::remove<CLASS>(llvm::Function & F) {                           \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    F.setMetadata(tag, nullptr);                                               \
    return true;                                                               \
  }                                                                            \
  template <>                                                                  \
  std::optional<CLASS> Metadata::get<CLASS>(llvm::Instruction & I) {           \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    if (not I.hasMetadata(tag)) {                                              \
      return std::nullopt;                                                     \
    }                                                                          \
    auto *metadata = dyn_cast_or_null<llvm::MDTuple>(I.getMetadata(tag));      \
    return (metadata == nullptr) ? std::nullopt                                \
                                 : std::make_optional(CLASS(*metadata));       \
  }                                                                            \
  template <>                                                                  \
  CLASS Metadata::get_or_add<CLASS>(llvm::Instruction & I) {                   \
    auto got = Metadata::get<CLASS>(I);                                        \
    if (got.has_value()) {                                                     \
      return got.value();                                                      \
    }                                                                          \
    auto &context = I.getContext();                                            \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    auto *metadata =                                                           \
        llvm::MDTuple::getDistinct(context,                                    \
                                   llvm::ArrayRef<llvm::Metadata *>({}));      \
    I.setMetadata(tag, metadata);                                              \
    return CLASS(*metadata);                                                   \
  }                                                                            \
  template <>                                                                  \
  bool Metadata::remove<CLASS>(llvm::Instruction & I) {                        \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    I.setMetadata(tag, nullptr);                                               \
    return true;                                                               \
  }                                                                            \
  template <>                                                                  \
  std::optional<CLASS> Metadata::get<CLASS>(MemOIRInst & I) {                  \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    if (not I.getCallInst().hasMetadata(tag)) {                                \
      return std::nullopt;                                                     \
    }                                                                          \
    auto *metadata =                                                           \
        dyn_cast_or_null<llvm::MDTuple>(I.getCallInst().getMetadata(tag));     \
    return (metadata == nullptr) ? std::nullopt                                \
                                 : std::make_optional(CLASS(*metadata));       \
  }                                                                            \
  template <>                                                                  \
  CLASS Metadata::get_or_add<CLASS>(MemOIRInst & I) {                          \
    auto got = Metadata::get<CLASS>(I.getCallInst());                          \
    if (got.has_value()) {                                                     \
      return got.value();                                                      \
    }                                                                          \
    auto &context = I.getCallInst().getContext();                              \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    auto *metadata =                                                           \
        llvm::MDTuple::getDistinct(context,                                    \
                                   llvm::ArrayRef<llvm::Metadata *>({}));      \
    I.getCallInst().setMetadata(tag, metadata);                                \
    return CLASS(*metadata);                                                   \
  }                                                                            \
  template <>                                                                  \
  bool Metadata::remove<CLASS>(MemOIRInst & I) {                               \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    I.getCallInst().setMetadata(tag, nullptr);                                 \
    return true;                                                               \
  }                                                                            \
  template <>                                                                  \
  std::optional<CLASS> Metadata::get<CLASS>(StructType & type,                 \
                                            unsigned field) {                  \
    auto *tuple = detail::get_field_metadata(type, field);                     \
    if (not tuple) {                                                           \
      return std::nullopt;                                                     \
    }                                                                          \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    auto *tagged_node = detail::find_tagged_metadata(*tuple, tag);             \
    if (not tagged_node) {                                                     \
      return std::nullopt;                                                     \
    }                                                                          \
    auto &metadata = MEMOIR_SANITIZE(                                          \
        dyn_cast_or_null<llvm::MDTuple>(tagged_node->getOperand(1).get()),     \
        "Malformed metadata.");                                                \
    return CLASS(metadata);                                                    \
  }                                                                            \
  template <>                                                                  \
  CLASS Metadata::get_or_add<CLASS>(StructType & type, unsigned field) {       \
    auto &tuple = detail::get_or_add_field_metadata(type, field);              \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    auto *tagged_node = detail::find_tagged_metadata(tuple, tag);              \
    if (not tagged_node) {                                                     \
      auto &context = type.getDefinition().getCallInst().getContext();         \
      auto *metadata_tag = llvm::MDString::get(context, tag);                  \
      auto *empty_tuple =                                                      \
          llvm::MDTuple::getDistinct(context,                                  \
                                     llvm::ArrayRef<llvm::Metadata *>({}));    \
      tagged_node = llvm::MDNode::getDistinct(                                 \
          context,                                                             \
          llvm::ArrayRef<llvm::Metadata *>({ metadata_tag, empty_tuple }));    \
      return CLASS(*empty_tuple);                                              \
    }                                                                          \
    auto &metadata = MEMOIR_SANITIZE(                                          \
        dyn_cast_or_null<llvm::MDTuple>(tagged_node->getOperand(1).get()),     \
        "Malformed metadata.");                                                \
    return CLASS(metadata);                                                    \
  }                                                                            \
  template <>                                                                  \
  bool Metadata::remove<CLASS>(StructType & type, unsigned field) {            \
    auto &tuple = detail::get_or_add_field_metadata(type, field);              \
    auto tag = detail::get_metadata_tag(MetadataKind::ENUM);                   \
    auto *tagged_node = detail::find_tagged_metadata(tuple, tag);              \
    if (not tagged_node) {                                                     \
      return false;                                                            \
    }                                                                          \
    vector<llvm::Metadata *> items = {};                                       \
    while (tuple.getNumOperands() > 0) {                                       \
      auto &operand = tuple.getOperand(tuple.getNumOperands() - 1);            \
      auto *op_metadata = operand.get();                                       \
      if (op_metadata == tagged_node) {                                        \
        tuple.pop_back();                                                      \
        break;                                                                 \
      } else {                                                                 \
        items.push_back(op_metadata);                                          \
      }                                                                        \
      tuple.pop_back();                                                        \
    }                                                                          \
    for (auto *item : items) {                                                 \
      tuple.push_back(item);                                                   \
    }                                                                          \
    return true;                                                               \
  }
#include "memoir/utility/Metadata.def"

llvm::MDTuple &Metadata::getMetadata() const {
  return *this->md;
}

} // namespace llvm::memoir
