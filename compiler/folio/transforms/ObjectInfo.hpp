#ifndef FOLIO_TRANSFORMS_OBJECTINFO_H
#define FOLIO_TRANSFORMS_OBJECTINFO_H

#include "llvm/Transforms/Utils/ValueMapper.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/transforms/Context.hpp"
#include "folio/transforms/NestedObject.hpp"

namespace folio {

struct ObjectInfo {

  template <typename T>
  using LocalMap =
      llvm::memoir::SmallMap<llvm::Value *, T, /* SmallSize = */ 2>;
  using Redefinitions = ContextMap<LocalMap<llvm::memoir::Set<NestedObject>>>;

  llvm::memoir::AllocInst *allocation;
  llvm::memoir::Vector<unsigned> offsets;
  Redefinitions redefinitions;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>> encoded;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Use *>> to_encode;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Use *>> to_addkey;
  llvm::memoir::Map<llvm::Value *, llvm::memoir::Set<llvm::Value *>>
      base_to_values;
  llvm::memoir::Map<llvm::Value *, llvm::memoir::Set<llvm::Use *>> base_to_uses;
  llvm::memoir::Map<llvm::Use *, llvm::Value *> use_to_base;

  ObjectInfo(llvm::memoir::AllocInst &alloc, llvm::ArrayRef<unsigned> offsets)
    : allocation(&alloc),
      offsets(offsets.begin(), offsets.end()),
      redefinitions{},
      encoded{},
      to_encode{},
      to_addkey{},
      base_to_values{},
      base_to_uses{},
      use_to_base{} {}
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
