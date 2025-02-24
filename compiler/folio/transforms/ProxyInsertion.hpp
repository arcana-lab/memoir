#include "llvm/Analysis/CallGraph.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"

namespace folio {

struct ObjectInfo {
  llvm::memoir::AllocInst *allocation;
  llvm::memoir::vector<unsigned> offsets;
  llvm::memoir::map<llvm::Function *, llvm::memoir::set<llvm::Value *>>
      redefinitions;
  llvm::memoir::map<llvm::Function *, llvm::memoir::set<llvm::Use *>> to_encode;
  llvm::memoir::map<llvm::Function *, llvm::memoir::set<llvm::Use *>> to_decode;
  llvm::memoir::map<llvm::Function *, llvm::memoir::set<llvm::Use *>> to_addkey;

  ObjectInfo(llvm::memoir::AllocInst &alloc, llvm::ArrayRef<unsigned> offsets)
    : allocation(&alloc),
      offsets(offsets.begin(), offsets.end()),
      redefinitions{},
      to_encode{},
      to_decode{},
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

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const ObjectInfo &info);
};

using Candidate = llvm::memoir::vector<ObjectInfo *>;

struct ProxyInsertion {
public:
  ProxyInsertion(llvm::Module &M);

  void analyze();

  bool transform();

protected:
  void gather_assoc_objects(
      llvm::memoir::vector<ObjectInfo> &allocations,
      llvm::memoir::AllocInst &alloc,
      llvm::memoir::Type &type,
      llvm::memoir::vector<unsigned> offsets = {},
      std::optional<llvm::memoir::SelectionMetadata> selection = {},
      unsigned selection_index = 0);

  llvm::Module &M;
  llvm::memoir::vector<ObjectInfo> objects;
  llvm::memoir::vector<Candidate> candidates;
};

} // namespace folio
