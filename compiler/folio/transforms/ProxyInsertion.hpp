#ifndef FOLIO_TRANSFORMS_PROXYINSERTION_H
#define FOLIO_TRANSFORMS_PROXYINSERTION_H

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"

namespace folio {

struct ObjectInfo {
  llvm::memoir::AllocInst *allocation;
  llvm::memoir::Vector<unsigned> offsets;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>>
      redefinitions;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>> encoded;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Use *>> to_encode;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Use *>> to_addkey;

  ObjectInfo(llvm::memoir::AllocInst &alloc, llvm::ArrayRef<unsigned> offsets)
    : allocation(&alloc),
      offsets(offsets.begin(), offsets.end()),
      redefinitions{},
      encoded{},
      to_encode{},
      to_addkey{} {}
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
   * @param delete_old, if we should delete information about the old function.
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

using Candidate = llvm::memoir::Vector<ObjectInfo *>;

struct ProxyInsertion {
public:
  using GetDominatorTree =
      std::function<llvm::DominatorTree &(llvm::Function &)>;
  using GetBoundsChecks =
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>;

  ProxyInsertion(llvm::Module &M,
                 GetDominatorTree get_dominator_tree,
                 GetBoundsChecks get_bounds_checks);

  void analyze();

  bool transform();

protected:
  void gather_assoc_objects(llvm::memoir::Vector<ObjectInfo> &allocations,
                            llvm::memoir::AllocInst &alloc,
                            llvm::memoir::Type &type,
                            llvm::memoir::Vector<unsigned> offsets = {});

  ObjectInfo *find_base_object(llvm::Value &V,
                               llvm::memoir::AccessInst &access);

  void gather_propagators(
      llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>>
          encoded,
      llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Use *>>
          to_decode);

  llvm::Module &M;
  llvm::memoir::Vector<ObjectInfo> objects;
  llvm::memoir::Vector<ObjectInfo> propagators;
  llvm::memoir::Vector<Candidate> candidates;

  GetDominatorTree get_dominator_tree;
  GetBoundsChecks get_bounds_checks;
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_PROXYINSERTION_H
