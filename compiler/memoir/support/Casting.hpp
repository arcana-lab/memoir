#ifndef MEMOIR_CASTING_H
#define MEMOIR_CASTING_H
#pragma once

#include <type_traits>

#include "llvm/Support/Casting.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

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

} // namespace llvm::memoir

#endif
