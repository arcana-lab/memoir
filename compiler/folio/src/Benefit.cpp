#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/Value.h"

#include "folio/Benefit.hpp"
#include "folio/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

// TODO: add more heuristics and create a framework for selecting between them.

static int heuristic(const Map<llvm::Function *, Set<llvm::Value *>> &encoded,
                     const Map<llvm::Function *, Set<llvm::Use *>> &to_encode,
                     const Map<llvm::Function *, Set<llvm::Use *>> &to_addkey) {

  int benefit = 0;
  for (const auto &[func, values] : encoded) {
    for (const auto *value : values) {
      for (auto &use_to_decode : value->uses()) {

        auto *user = use_to_decode.getUser();

        if (to_encode.count(func)) {
          for (const auto *use_to_encode : to_encode.at(func)) {
            if (&use_to_decode == use_to_encode) {
              ++benefit;
            }
          }
        }

        if (to_addkey.count(func)) {
          for (const auto *use_to_addkey : to_addkey.at(func)) {
            if (&use_to_decode == use_to_addkey) {
              ++benefit;
            }
          }
        }

        auto *cmp = dyn_cast<llvm::CmpInst>(user);
        if (cmp and cmp->isEquality()) {
          if (values.contains(cmp->getOperand(0))
              and values.contains(cmp->getOperand(1))) {
            ++benefit;
          }
        }
      }
    }
  }

  return benefit;
}

int benefit(llvm::ArrayRef<const ObjectInfo *> candidate) {
  // Compute the benefit of this candidate.

  // Merge encoded values.
  Map<llvm::Function *, Set<llvm::Value *>> encoded = {};
  for (const auto *info : candidate) {
    for (const auto &[func, values] : info->encoded) {
      encoded[func].insert(values.begin(), values.end());
    }
  }

  // Perform a forward data flow analysis on the encoded values.
  forward_analysis(encoded);

  // Merge uses.
  Map<llvm::Function *, Set<llvm::Use *>> to_encode = {}, to_addkey = {};
  for (const auto *info : candidate) {
    for (const auto &[func, uses] : info->to_encode) {
      to_encode[func].insert(uses.begin(), uses.end());
    }

    for (const auto &[func, uses] : info->to_addkey) {
      to_addkey[func].insert(uses.begin(), uses.end());
    }
  }

  // Perform a "what if?" analysis.
  return heuristic(encoded, to_encode, to_addkey);
}

} // namespace folio
