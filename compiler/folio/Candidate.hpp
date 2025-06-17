#ifndef FOLIO_TRANSFORMS_CANDIDATE_H
#define FOLIO_TRANSFORMS_CANDIDATE_H

#include "llvm/IR/Dominators.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/CoalesceUses.hpp"
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
  struct Mapping {
    Mapping() : _alloc(NULL), _globals{}, _locals{} {}

    llvm::Value &alloc() const {
      return *this->_alloc;
    }

    void alloc(llvm::Value &V) {
      this->_alloc = &V;
    }

    llvm::GlobalVariable &global(llvm::Value *base) const {
      return *this->_globals.at(base);
    }

    void global(llvm::Value *base, llvm::GlobalVariable &GV) {
      this->_globals[base] = &GV;
    }

    Map<llvm::Value *, llvm::GlobalVariable *> &globals() {
      return this->_globals;
    }

    const Map<llvm::Value *, llvm::GlobalVariable *> &globals() const {
      return this->_globals;
    }

    llvm::AllocaInst *local(llvm::Function &F) const {
      auto found = this->_locals.find(&F);
      if (found == this->_locals.end()) {
        return NULL;
      }

      return found->second;
    }

    void local(llvm::Function &func, llvm::AllocaInst &alloca) {
      this->_locals[&func] = &alloca;
    }

    llvm::Value *_alloc;
    Map<llvm::Value *, llvm::GlobalVariable *> _globals;
    Map<llvm::Function *, llvm::AllocaInst *> _locals;
  };
  Mapping encoder, decoder;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Candidate &uses);
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_CANDIDATE_H
