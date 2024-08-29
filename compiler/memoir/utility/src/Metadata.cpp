#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

std::string get_metadata_tag(MetadataKind kind) {
  switch (kind) {
#define METADATA(ENUM, STR, CLASS)                                             \
  case MetadataKind::ENUM:                                                     \
    return STR;
#include "memoir/utility/Metadata.def"
    default:
      return "";
  }
}

#define METADATA(ENUM, STR, CLASS)                                             \
  template <>                                                                  \
  std::optional<CLASS> Metadata::get(llvm::Function &F) {                      \
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
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
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
    auto *metadata =                                                           \
        llvm::MDTuple::getDistinct(context,                                    \
                                   llvm::ArrayRef<llvm::Metadata *>({}));      \
    F.setMetadata(tag, metadata);                                              \
    return CLASS(*metadata);                                                   \
  }                                                                            \
  template <>                                                                  \
  bool Metadata::remove<CLASS>(llvm::Function & F) {                           \
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
    F.setMetadata(tag, nullptr);                                               \
    return true;                                                               \
  }                                                                            \
  template <>                                                                  \
  std::optional<CLASS> Metadata::get<CLASS>(llvm::Instruction & I) {           \
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
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
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
    auto *metadata =                                                           \
        llvm::MDTuple::getDistinct(context,                                    \
                                   llvm::ArrayRef<llvm::Metadata *>({}));      \
    I.setMetadata(tag, metadata);                                              \
    return CLASS(*metadata);                                                   \
  }                                                                            \
  template <>                                                                  \
  bool Metadata::remove<CLASS>(llvm::Instruction & I) {                        \
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
    I.setMetadata(tag, nullptr);                                               \
    return true;                                                               \
  }                                                                            \
  template <>                                                                  \
  std::optional<CLASS> Metadata::get<CLASS>(MemOIRInst & I) {                  \
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
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
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
    auto *metadata =                                                           \
        llvm::MDTuple::getDistinct(context,                                    \
                                   llvm::ArrayRef<llvm::Metadata *>({}));      \
    I.getCallInst().setMetadata(tag, metadata);                                \
    return CLASS(*metadata);                                                   \
  }                                                                            \
  template <>                                                                  \
  bool Metadata::remove<CLASS>(MemOIRInst & I) {                               \
    auto tag = get_metadata_tag(MetadataKind::ENUM);                           \
    I.getCallInst().setMetadata(tag, nullptr);                                 \
    return true;                                                               \
  }
#include "memoir/utility/Metadata.def"

llvm::MDTuple &Metadata::getMetadata() const {
  return *this->md;
}

} // namespace llvm::memoir
