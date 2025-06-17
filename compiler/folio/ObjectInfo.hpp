#ifndef FOLIO_TRANSFORMS_OBJECTINFO_H
#define FOLIO_TRANSFORMS_OBJECTINFO_H

#include "llvm/Transforms/Utils/ValueMapper.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/Context.hpp"
#include "folio/NestedObject.hpp"
#include "folio/Utilities.hpp"

namespace folio {

struct ObjectInfo {

  using Redefinitions = ContextMap<LocalMap<llvm::memoir::Set<NestedObject>>>;

  /** The single static allocation of this object. */
  llvm::memoir::AllocInst *allocation;

  /** Nested offset into the allocation */
  Vector<unsigned> offsets;

  /** Interprocedural redefinitions of the allocation. */
  Redefinitions redefinitions;

  /** Tracks the encoded values read from this object. */
  Map<llvm::Function *, Set<llvm::Value *>> encoded;
  /** Tracks the uses to encode. */
  Map<llvm::Function *, Set<llvm::Use *>> to_encode;
  /** Tracks the uses to encode. */
  Map<llvm::Function *, Set<llvm::Use *>> to_addkey;

  /** Tracks the base for each encoded value. */
  LocalMap<Set<llvm::Value *>> base_encoded;
  /** Reverse mapping of {@link ObjectInfo#base_encoded}. */
  Map<llvm::Value *, llvm::Value *> encoded_base;

  /** Tracks the base for each use to encode. */
  LocalMap<Set<llvm::Use *>> base_to_encode;
  /** Reverse mapping of {@link ObjectInfo#base_to_encode}. */
  Map<llvm::Use *, llvm::Value *> to_encode_base;

  /** Tracks the base for each use to addkey. */
  LocalMap<Set<llvm::Use *>> base_to_addkey;
  /** Reverse mapping of {@link ObjectInfo#base_to_addkey}. */
  Map<llvm::Use *, llvm::Value *> to_addkey_base;

  ObjectInfo(llvm::memoir::AllocInst &alloc, llvm::ArrayRef<unsigned> offsets)
    : allocation(&alloc),
      offsets(offsets.begin(), offsets.end()),
      redefinitions{},
      encoded{},
      to_encode{},
      to_addkey{},
      base_encoded{},
      encoded_base{},
      base_to_encode{},
      to_encode_base{},
      base_to_addkey{},
      to_addkey_base{} {}
  ObjectInfo(llvm::memoir::AllocInst &alloc) : ObjectInfo(alloc, {}) {}

  /**
   * Get the type of the referenced collection.
   */
  llvm::memoir::Type &get_type() const;

  void analyze();

  /**
   * Update the analysis results for this object info.
   * @param old_func, the old function.
   * @param new_func, the new function.
   * @param vmap, a map from original values to cloned values.
   * @param delete_old, if we should delete information about the old
   * function.
   */
  void update(llvm::Function &old_func,
              llvm::Function &new_func,
              llvm::ValueToValueMapTy &vmap,
              bool delete_old = false);

  /**
   * Compute the benefit of having these two collections share a proxy space.
   */
  uint32_t compute_heuristic(const ObjectInfo &other) const;

  /**
   * Check if the given value is a redefinition of the object.
   */
  bool is_redefinition(llvm::Value &V) const;

  /**
   * Check if this nested object is a propagator.
   */
  bool is_propagator() const;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const ObjectInfo &info);
};

} // namespace folio

#endif
