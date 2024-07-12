#ifndef COMMON_METADATA_H
#define COMMON_METADATA_H
#pragma once

#include <optional>
#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

enum class MetadataKind {
#define METADATA(ENUM, STR, CLASS) ENUM,
#include "memoir/utility/Metadata.def"
  MD_NONE,
};

// Base class to extend for custom metadata.
struct Metadata {
public:
  // Static helper methods.
  template <typename T>
  static std::optional<T> get(llvm::Function &F);

  template <typename T>
  static T get_or_add(llvm::Function &F);

  template <typename T>
  static bool remove(llvm::Function &F);

  template <typename T>
  static std::optional<T> get(llvm::Instruction &I);

  template <typename T>
  static T get_or_add(llvm::Instruction &I);

  template <typename T>
  static bool remove(llvm::Instruction &I);

  // Constructor.
  Metadata(llvm::MDTuple &md) : md(&md) {}

  /**
   * @return the LLVM MDTuple wrapped by this object.
   */
  llvm::MDTuple &getMetadata() const;

protected:
  llvm::MDTuple *md;
};

struct LiveOutMetadata : public Metadata {
public:
  // Constructor.
  LiveOutMetadata(llvm::MDTuple &md) : Metadata(md) {}

  /**
   * Get the corresponding argument number that is a live-out for.
   * NOTE: this assumes a single ReturnInst for each LLVM Function.
   *
   * @return the argument number
   */
  unsigned getArgNo() const;
  llvm::Metadata &getArgNoMD() const;
  const llvm::MDOperand &getArgNoMDOperand() const;

  /**
   * Set the argument number.
   *
   * @param argument_number the argument number
   */
  void setArgNo(unsigned argument_number);
};

} // namespace llvm::memoir

#endif
