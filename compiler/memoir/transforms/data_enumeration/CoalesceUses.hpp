#ifndef FOLIO_TRANSFORMS_COALESCEUSES_H
#define FOLIO_TRANSFORMS_COALESCEUSES_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"

#include "Utilities.hpp"

namespace memoir {

struct CoalescedUses : public Vector<llvm::Use *> {
protected:
  using Base = Vector<llvm::Use *>;

  llvm::Value *_value;
  llvm::Value *_base;

public:
  CoalescedUses(llvm::Use *use, llvm::Value *base = NULL)
    : Base{ use },
      _value(use->get()),
      _base{ base } {}
  CoalescedUses(llvm::ArrayRef<llvm::Use *> uses, llvm::Value *base = NULL)
    : Base(uses.begin(), uses.end()),
      _value(uses.front()->get()),
      _base{ base } {}

  llvm::Value &value() const {
    return *this->_value;
  }

  void value(llvm::Value &value) {
    this->_value = &value;
  }

  llvm::Value *base() const {
    return this->_base;
  }

  void base(llvm::Value *base) {
    this->_base = base;
  }

  llvm::Instruction *insertion_point() const {
    llvm::Instruction *program_point = nullptr;

    auto *use = this->Base::front();
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());

    if (user) {
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        auto *incoming_block = phi->getIncomingBlock(*use);
        // TODO: this may be unsound if the terminator is conditional.
        program_point = incoming_block->getTerminator();
      } else {
        program_point = user;
      }
    } else {
      MEMOIR_UNREACHABLE("Failed to find a point to decode the value!");
    }

    return program_point;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const CoalescedUses &uses);
};

void coalesce(
    Vector<CoalescedUses> &coalesced,
    const LocalMap<Set<llvm::Use *>> &uses,
    std::function<llvm::DominatorTree &(llvm::Function &)> get_dominator_tree);

} // namespace memoir

#endif // FOLIO_TRANSFORMS_COALESCEUSES_H
