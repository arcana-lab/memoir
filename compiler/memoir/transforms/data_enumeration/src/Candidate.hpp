#ifndef FOLIO_TRANSFORMS_CANDIDATE_H
#define FOLIO_TRANSFORMS_CANDIDATE_H

#include "llvm/IR/Dominators.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "CoalesceUses.hpp"
#include "Mapping.hpp"
#include "ObjectInfo.hpp"
#include "Utilities.hpp"

namespace memoir {

struct Candidate : public Vector<ObjectInfo *> {
protected:
  Type *_key_type;

public:
  using Base = Vector<ObjectInfo *>;

  Candidate(Type &key_type, llvm::ArrayRef<ObjectInfo *> objects = {})
    : Base(objects.begin(), objects.end()),
      _key_type(&key_type) {}

  // Type and program information.
  llvm::Module &module() const;
  llvm::Function &function() const;

  Type &key_type() const;
  Type &encoder_type() const;
  Type &decoder_type() const;

  // The uses prepared for transformation.
  Map<ObjectInfo *, Map<llvm::Function *, Set<llvm::Use *>>> to_decode,
      to_encode, to_addkey;

  // Intermediate analysis results.
  Map<ObjectInfo *, Map<llvm::Function *, Set<llvm::Value *>>> encoded;

public:
  // Unique identifier.
  int id;

  // Cost model.
  int benefit;

  // Transform.
protected:
  void update(llvm::Function &old_func,
              llvm::Function &new_Func,
              llvm::ValueToValueMapTy &vmap,
              bool delete_old = false);

public:
  /** Global information for this candidate's encoder. */
  Mapping encoder;
  /** Global information for this candidate's decoder. */
  Mapping decoder;

  bool build_decoder() const;
  bool build_encoder() const;

  // Print.
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Candidate &uses);
};

} // namespace memoir

#endif // FOLIO_TRANSFORMS_CANDIDATE_H
