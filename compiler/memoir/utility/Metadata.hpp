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
   * @return the number of live out values.
   */
  unsigned getNumLiveOuts() const;

  /**
   * @return the LLVM Value corresponding to the argument passed into the
   * function.
   */
  llvm::Value &getArgument(unsigned i) const;
  llvm::Metadata &getArgumentMD(unsigned i) const;
  const llvm::MDOperand &getArgumentMDOperand(unsigned i) const;

  /**
   * @param i get the i'th live out.
   * @return the live out LLVM Value of the function.
   */
  llvm::Value &getLiveOut(unsigned i) const;
  llvm::Metadata &getLiveOutMD(unsigned i) const;
  const llvm::MDOperand &getLiveOutMDOperand(unsigned i) const;

  /**
   * Add an Argument-LiveOut pair to the metadata.
   *
   * @param argument the argument that has a live-out.
   * @param live_out the live-out value.
   */
  void addLiveOut(llvm::Value &argument, llvm::Value &live_out) const;
};

} // namespace llvm::memoir

#endif
