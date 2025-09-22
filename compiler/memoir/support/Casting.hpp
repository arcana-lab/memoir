#ifndef MEMOIR_CASTING_H
#define MEMOIR_CASTING_H

#include <type_traits>

#include "llvm/Support/Casting.h"

#include "memoir/support/Assert.hpp"

namespace memoir {

struct MemOIRInst;
struct Keyword;

using llvm::isa;
using llvm::isa_and_nonnull;

using llvm::cast;
using llvm::cast_or_null;

using llvm::dyn_cast;
using llvm::dyn_cast_or_null;

// Functions to dyn_cast an llvm::Instruction to a MemOIRInst
template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<llvm::Value, From>, bool> = true>
To *into(From *V);

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<llvm::Value, From>, bool> = true>
To *into(From &V);

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<llvm::Value, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, From>, bool> = true>
To *into(From *I);

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<llvm::Value, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<MemOIRInst, From>, bool> = true>
To *into(From &I);

// Functions to check type of Keyword
template <class To,
          std::enable_if_t<std::is_base_of_v<Keyword, To>, bool> = true,
          std::enable_if_t<not std::is_same_v<Keyword, To>, bool> = true>
std::optional<To> into(llvm::Use &U);

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<Keyword, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<Keyword, From>, bool> = true>
std::optional<To> try_cast(const From &kw);

} // namespace memoir

#endif
