#ifndef FOLIO_TRANSFORMS_CANDIDATE_H
#define FOLIO_TRANSFORMS_CANDIDATE_H

#include "llvm/IR/Dominators.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/CoalesceUses.hpp"
#include "folio/Mapping.hpp"
#include "folio/ObjectInfo.hpp"
#include "folio/Utilities.hpp"

namespace folio {

struct Candidate : public Vector<ObjectInfo *> {
protected:
  llvm::memoir::Type *_key_type;

public:
  using Base = Vector<ObjectInfo *>;

  Candidate(llvm::memoir::Type &key_type,
            llvm::ArrayRef<ObjectInfo *> objects = {})
    : Base(objects.begin(), objects.end()),
      _key_type(&key_type) {}

  // Type and program information.
  llvm::Module &module() const;
  llvm::Function &function() const;

  llvm::memoir::Type &key_type() const;
  llvm::memoir::Type &encoder_type() const;
  llvm::memoir::Type &decoder_type() const;

  // The uses prepared for transformation.
  Map<ObjectInfo *, Map<llvm::Function *, Set<llvm::Use *>>> to_decode,
      to_encode, to_addkey;

  // Intermediate analysis results.
  Map<ObjectInfo *, Map<llvm::Function *, Set<llvm::Value *>>> encoded;

public:
  void gather_uses(const Map<ObjectInfo *, SmallVector<ObjectInfo *>> &equiv);

  // Cost model.
  int benefit;

  // Transform.
protected:
  llvm::Function *addkey_function = NULL;
  llvm::FunctionCallee addkey_callee();

  void update(llvm::Function &old_func,
              llvm::Function &new_Func,
              llvm::ValueToValueMapTy &vmap,
              bool delete_old = false);

public:
  /** Global information for this candidate's encoder. */
  Mapping encoder;
  /** Global information for this candidate's decoder. */
  Mapping decoder;

  llvm::Instruction &construction_point(llvm::DominatorTree &domtree) const;
  bool build_decoder() const;
  bool build_encoder() const;

  /** Check if the enumeration has the given value */
  llvm::Instruction &has_value(llvm::memoir::MemOIRBuilder &builder,
                               llvm::Value &value,
                               llvm::Value *enc_ptr);
  /** Decode the given value */
  llvm::Value &decode_value(llvm::memoir::MemOIRBuilder &builder,
                            llvm::Value &value,
                            llvm::Value *dec_ptr);
  /** Encode the given value */
  llvm::Value &encode_value(llvm::memoir::MemOIRBuilder &builder,
                            llvm::Value &value,
                            llvm::Value *enc_ptr);
  /** Add the given value to the enumeration */
  llvm::Value &add_value(llvm::memoir::MemOIRBuilder &builder,
                         llvm::Value &value,
                         llvm::Value *enc_ptr,
                         llvm::Value *dec_ptr);

  // Print.
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Candidate &uses);
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_CANDIDATE_H
