#ifndef FOLIO_TRANSFORMS_CANDIDATE_H
#define FOLIO_TRANSFORMS_CANDIDATE_H

#include "llvm/IR/Dominators.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/CoalesceUses.hpp"
#include "folio/Mapping.hpp"
#include "folio/ObjectInfo.hpp"
#include "folio/Utilities.hpp"

namespace folio {

struct Candidate : public Vector<ObjectInfo *> {
  using Base = Vector<ObjectInfo *>;

  Candidate()
    : Base{},
      decoded{},
      encoded{},
      added{},
      encoded_values{},
      encoder(),
      decoder() {}

  // The uses prepared for transformation.
  Vector<CoalescedUses> decoded, encoded, added;
  Map<llvm::Function *, LocalMap<Set<llvm::Value *>>> encoded_values;

  llvm::Module &module() const;
  llvm::Function &function() const;

  llvm::memoir::Type &key_type() const;

  llvm::Instruction &construction_point(llvm::DominatorTree &domtree) const;

  bool build_decoder() const;
  bool build_encoder() const;

  void gather_uses(Map<llvm::Function *, LocalMap<Set<llvm::Value *>>> &encoded,
                   LocalMap<Set<llvm::Use *>> &to_decode,
                   LocalMap<Set<llvm::Use *>> &to_encode,
                   LocalMap<Set<llvm::Use *>> &to_addkey) const;

  void optimize(
      std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree,
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>
          get_bounds_checks);

  // Information about the en/decoder mappings.
  Map<ObjectInfo *, Mapping> encoder, decoder;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Candidate &uses);
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_CANDIDATE_H
