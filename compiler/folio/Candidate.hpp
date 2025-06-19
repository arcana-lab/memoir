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
public:
  using Base = Vector<ObjectInfo *>;

  Candidate()
    : Base{},
      decoded{},
      encoded{},
      added{},
      encoded_values{},
      encoder(),
      decoder(),
      addkey_function(NULL) {}

  // The uses prepared for transformation.
  Vector<CoalescedUses> decoded, encoded, added;
  Map<llvm::Function *, LocalMap<Set<llvm::Value *>>> encoded_values;

  // Type and program information.
  llvm::Module &module() const;
  llvm::Function &function() const;

  llvm::memoir::Type &key_type() const;
  llvm::memoir::Type &encoder_type() const;
  llvm::memoir::Type &decoder_type() const;

  // Cost model.
  int benefit;

  /** Optimize this candidate's uses. */
  void optimize(
      std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree,
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>
          get_bounds_checks);

  /** Global information for this candidate's encoder. */
  Mapping encoder;
  /** Global information for this candidate's decoder. */
  Mapping decoder;

  // Transform.
protected:
  llvm::Function *addkey_function;
  llvm::FunctionCallee addkey_callee();

public:
  llvm::Instruction &construction_point(llvm::DominatorTree &domtree) const;
  bool build_decoder() const;
  bool build_encoder() const;

  /** Check if the enumeration has the given value */
  llvm::Instruction &has_value(llvm::memoir::MemOIRBuilder &builder,
                               llvm::Value &value,
                               llvm::Value *base);
  /** Decode the given value */
  llvm::Value &decode_value(llvm::memoir::MemOIRBuilder &builder,
                            llvm::Value &value,
                            llvm::Value *base);
  /** Encode the given value */
  llvm::Value &encode_value(llvm::memoir::MemOIRBuilder &builder,
                            llvm::Value &value,
                            llvm::Value *base);
  /** Add the given value to the enumeration */
  llvm::Value &add_value(llvm::memoir::MemOIRBuilder &builder,
                         llvm::Value &value,
                         llvm::Value *base);

  void gather_uses(Map<llvm::Function *, LocalMap<Set<llvm::Value *>>> &encoded,
                   LocalMap<Set<llvm::Use *>> &to_decode,
                   LocalMap<Set<llvm::Use *>> &to_encode,
                   LocalMap<Set<llvm::Use *>> &to_addkey) const;

  // Print.
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Candidate &uses);
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_CANDIDATE_H
