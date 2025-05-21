#ifndef FOLIO_TRANSFORMS_COALESCEUSES_H
#define FOLIO_TRANSFORMS_COALESCEUSES_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/transforms/ProxyInsertion.hpp"

namespace folio {

struct CoalescedUses : public llvm::memoir::Vector<llvm::Use *> {
protected:
  using Base = llvm::memoir::Vector<llvm::Use *>;

  llvm::Value *_value;

public:
  CoalescedUses(llvm::Use *use) : Base{ use }, _value(use->get()) {}
  CoalescedUses(llvm::ArrayRef<llvm::Use *> uses)
    : Base(uses.begin(), uses.end()),
      _value(uses.front()->get()) {}

  llvm::Value &value() const {
    return *this->_value;
  }

  void value(llvm::Value &value) {
    this->_value = &value;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const CoalescedUses &uses);
};

void coalesce(llvm::memoir::Vector<CoalescedUses> &decoded,
              llvm::memoir::Vector<CoalescedUses> &encoded,
              llvm::memoir::Vector<CoalescedUses> &added,
              const llvm::memoir::Set<llvm::Use *> &to_decode,
              const llvm::memoir::Set<llvm::Use *> &to_encode,
              const llvm::memoir::Set<llvm::Use *> &to_addkey,
              ProxyInsertion::GetDominatorTree get_dominator_tree);

} // namespace folio

#endif // FOLIO_TRANSFORMS_COALESCEUSES_H
