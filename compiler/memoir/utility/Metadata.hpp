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

#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

enum class MetadataKind {
  MD_STRUCT_FIELDS,
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

  template <typename T>
  static std::optional<T> get(MemOIRInst &I);

  template <typename T>
  static T get_or_add(MemOIRInst &I);

  template <typename T>
  static bool remove(MemOIRInst &I);

  template <typename T>
  static std::optional<T> get(StructType &type, unsigned field);

  template <typename T>
  static T get_or_add(StructType &type, unsigned field);

  template <typename T>
  static bool remove(StructType &type, unsigned field);

  /**
   * @return the metadata kind as a string.
   */
  template <typename T>
  static std::string get_kind();

  /**
   * @returns the string held by the metadata
   */
  static std::string to_string(llvm::Metadata &metadata);

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

struct SelectionMetadata : public Metadata {
public:
  // Constructor.
  SelectionMetadata(llvm::MDTuple &md) : Metadata(md) {}

  /**
   * Get the identifier for the selection.
   *
   * @param i the index of the dimension
   * @return the selection identifier as a string
   */
  std::string getImplementation(unsigned i = 0) const;
  llvm::Metadata &getImplementationMD(unsigned i = 0) const;
  const llvm::MDOperand &getImplementationMDOperand(unsigned i = 0) const;

  struct iterator {
  public:
    iterator(const llvm::MDOperand *op) : op(op) {}

    iterator &operator++() {
      op = std::next(op);
      return *this;
    }

    std::string operator*() {
      return Metadata::to_string(*this->op->get());
    }

    friend bool operator==(const iterator &lhs, const iterator &rhs) {
      return lhs.op == rhs.op;
    }

  protected:
    const llvm::MDOperand *op;
  };

  /**
   * @return an iterator over the selected implementations.
   */
  llvm::iterator_range<iterator> implementations();
  iterator impl_begin();
  iterator impl_end();

  /**
   * Set the selected implementation.
   *
   * @param ID the selection identifier
   * @param i the index of the dimension
   */
  void setImplementation(std::string id, unsigned i = 0);
};

struct TempArgumentMetadata : public Metadata {
public:
  // Constructor.
  TempArgumentMetadata(llvm::MDTuple &md) : Metadata(md) {}
};

} // namespace llvm::memoir

#endif
