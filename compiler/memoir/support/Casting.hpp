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
template <class To, class From>
To *into(From *V);

template <class To, class From>
To *into(From &V);

template <class To, class From>
To *into(From *I);

template <class To, class From>
To *into(From &I);

// Functions to check type of Keyword
template <class To>
std::optional<To> into(llvm::Use &U);

template <class To, class From>
std::optional<To> try_cast(const From &kw);

} // namespace memoir

#endif
