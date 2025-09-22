#include "memoir/utility/Metadata.hpp"

namespace memoir {

std::string Metadata::to_string(llvm::Metadata &metadata) {
  if (auto *md_constant = dyn_cast<llvm::ConstantAsMetadata>(&metadata)) {
    auto *constant = md_constant->getValue();
    auto &constant_as_data_array = MEMOIR_SANITIZE(
        dyn_cast_or_null<llvm::ConstantDataArray>(constant),
        "Malformed metadata, expected an llvm::ConstantDataArray");

    return constant_as_data_array.getAsString().str();

  } else if (auto *md_string = dyn_cast<llvm::MDString>(&metadata)) {
    return md_string->getString().str();
  }

  MEMOIR_UNREACHABLE("Expected the metadata to contain a string.");
}

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
  std::string Metadata::get_kind<CLASS>() {                                    \
    return STR;                                                                \
  }
#include "memoir/utility/Metadata.def"

llvm::MDTuple &Metadata::getMetadata() const {
  return *this->md;
}

} // namespace memoir
