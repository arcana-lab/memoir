#ifndef FOLIO_TRANSFORMS_CANDIDATE_H
#define FOLIO_TRANSFORMS_CANDIDATE_H

#include "llvm/IR/Dominators.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/transforms/CoalesceUses.hpp"
#include "folio/transforms/ObjectInfo.hpp"

namespace folio {

struct Candidate : public llvm::memoir::Vector<ObjectInfo *> {
  using Base = typename llvm::memoir::Vector<ObjectInfo *>;

  Candidate() : Base{}, decoded{}, encoded{}, added{}, encoded_values{} {}

  // The uses prepared for transformation.
  llvm::memoir::Vector<CoalescedUses> decoded, encoded, added;
  llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>>
      encoded_values;

  llvm::Function &function() const;

  llvm::memoir::Type &key_type() const;

  llvm::Instruction &construction_point(llvm::DominatorTree &domtree) const;

  bool build_decoder() const;
  bool build_encoder() const;

  void gather_uses(llvm::memoir::Map<llvm::Function *,
                                     llvm::memoir::Set<llvm::Value *>> &encoded,
                   llvm::memoir::Set<llvm::Use *> &to_decode,
                   llvm::memoir::Set<llvm::Use *> &to_encode,
                   llvm::memoir::Set<llvm::Use *> &to_addkey) const;

  void optimize(
      std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree,
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>
          get_bounds_checks);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Candidate &uses);
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_CANDIDATE_H
