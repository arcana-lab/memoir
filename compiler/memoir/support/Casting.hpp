#ifndef MEMOIR_CASTING_H
#define MEMOIR_CASTING_H
#pragma once

#include <type_traits>

#include "llvm/Support/Casting.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Properties.hpp"

#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

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

// Functions to check type of Property
template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<Property, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<Property, From>, bool> = true>
bool isa(const From &p) {
  return To::class_of(p);
}

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<Property, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<Property, From>, bool> = true>
To cast(const From &p) {
  return To(p.getPropertyInst());
}

template <class To,
          class From,
          std::enable_if_t<std::is_base_of_v<Property, To>, bool> = true,
          std::enable_if_t<std::is_base_of_v<Property, From>, bool> = true>
std::optional<To> try_cast(const From &p) {
  if (isa<To>(p)) {
    return To(p.getPropertyInst());
  } else {
    return std::nullopt;
  }
}

} // namespace llvm::memoir

#endif
