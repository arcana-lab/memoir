#ifndef MEMOIR_CASTING_H
#define MEMOIR_CASTING_H

#include <type_traits>

#include "memoir/support/Assert.hpp"
#include "memoir/support/Concepts.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

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
namespace detail {
MemOIRInst *get_memoir(llvm::Instruction &inst);
}

template <Derived<MemOIRInst> To, Derived<llvm::Value> From>
To *into(From *V) {
  auto *I = dyn_cast_or_null<llvm::Instruction>(V);
  if (I == nullptr) {
    return nullptr;
  }
  auto *memoir_inst = detail::get_memoir(*I);
  return dyn_cast_or_null<To>(memoir_inst);
}

template <Derived<MemOIRInst> To, Derived<llvm::Value> From>
To *into(From &V) {
  return into<To>(&V);
}

template <Derived<llvm::Value> To, Derived<MemOIRInst> From>
To *into(From *I) {
  return dyn_cast_or_null<To>(&I->getCallInst());
}

template <Derived<llvm::Value> To, Derived<MemOIRInst> From>
To *into(From &I) {
  return into<To>(&I);
}

// Functions to check type of Keyword
template <Derived<Keyword> To, Derived<Keyword> From>
std::optional<To> try_cast(const From &kw) {
  if (To::classof(kw))
    return To(kw.getAsUse());
  return {};
}

} // namespace memoir

#endif
