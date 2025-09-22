#include "memoir/support/Casting.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Keywords.hpp"

namespace memoir {

// Functions to dyn_cast an llvm::Instruction to a MemOIRInst
template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<llvm::Value, From>, bool> = true>
To *into(From *V) {
  auto *I = dyn_cast_or_null<llvm::Instruction>(V);
  if (I == nullptr) {
    return nullptr;
  }
  auto *memoir_inst = MemOIRInst::get(*I);
  return dyn_cast_or_null<To>(memoir_inst);
}

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<llvm::Value, From>, bool> = true>
To *into(From &V) {
  auto *I = dyn_cast_or_null<llvm::Instruction>(&V);
  if (I == nullptr) {
    return nullptr;
  }
  auto *memoir_inst = MemOIRInst::get(*I);
  return dyn_cast_or_null<To>(memoir_inst);
}

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<llvm::Value, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, From>, bool> = true>
To *into(From *I) {
  return dyn_cast_or_null<To>(&I->getCallInst());
}

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<llvm::Value, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, From>, bool> = true>
To *into(From &I) {
  return dyn_cast_or_null<To>(&I.getCallInst());
}

// Functions to check type of Keyword
template <class To,
          std::enable_if_t<std::is_base_of_v<Keyword, To>, bool> = true,
          std::enable_if_t<not std::is_same_v<Keyword, To>, bool> = true>
std::optional<To> into(llvm::Use &U) {
  auto *value = U.get();
  auto *data = dyn_cast_or_null<llvm::ConstantDataArray>(value);
  if (not data) {
    return std::nullopt;
  }
  if (not data->isCString()) {
    return std::nullopt;
  }
  auto str = data->getAsCString();
  if (not str.starts_with(Keyword::PREFIX)) {
    return std::nullopt;
  }
#define KEYWORD(STR, CLASS)                                                    \
  else if (str.ends_with(#STR)) {                                              \
    return CLASS(U);                                                           \
  }
#include "memoir/ir/Keywords.def"
  else {
    return std::nullopt;
  }
}

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<Keyword, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<Keyword, From>, bool> = true>
std::optional<To> try_cast(const From &kw) {
  if (To::classof(kw)) {
    return To(kw.getAsUse());
  }

  return {};
}

} // namespace memoir
