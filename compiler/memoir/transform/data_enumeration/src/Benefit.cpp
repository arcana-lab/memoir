#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/Value.h"

#include "memoir/support/Casting.hpp"

#include "Benefit.hpp"
#include "Utilities.hpp"

using namespace memoir;

namespace memoir {

// TODO: add more heuristics and create a framework for selecting between them.

static Heuristic heuristic(
    const Map<llvm::Function *, Set<llvm::Use *>> &to_decode,
    const Map<llvm::Function *, Set<llvm::Use *>> &to_encode,
    const Map<llvm::Function *, Set<llvm::Use *>> &to_addkey) {

  Heuristic heuristic;
  auto &benefit = heuristic.benefit;
  auto &cost = heuristic.cost;

  // Are there any redundant translations?
  for (const auto &[func, uses] : to_decode) {
    for (auto *use : uses) {
      int old_benefit = benefit;

      for (const auto &to_handle : { to_encode, to_addkey })
        if (to_handle.contains(func))
          if (to_handle.at(func).contains(use))
            ++benefit;

      auto *cmp = dyn_cast<llvm::CmpInst>(use->getUser());
      if (cmp and cmp->isEquality())
        if (uses.contains(&cmp->getOperandUse(0))
            and uses.contains(&cmp->getOperandUse(1)))
          ++benefit;

      if (auto *phi = dyn_cast<llvm::PHINode>(use->getUser())) {
        for (const auto &to_handle : { to_encode, to_addkey })
          if (to_handle.contains(func))
            for (auto &phi_use : phi->uses())
              if (to_handle.at(func).contains(&phi_use))
                ++benefit;
      }

      // If we couldn't handle this use, then it will cost us.
      if (benefit == old_benefit)
        ++cost;
    }
  }

  // Are there any shared values?
  Set<llvm::Value *> seen;
  for (const auto &to_handle : { to_decode, to_encode, to_addkey })
    for (const auto &[func, uses] : to_handle) {
      if (func->getName().contains("add_constraint")) {
        print_uses(uses);
      }
      for (auto *use : uses) {
        if (seen.contains(use->get()))
          ++benefit;
        else
          seen.insert(use->get());
      }
    }

  return heuristic;
}

Heuristic benefit(llvm::ArrayRef<const ObjectInfo *> candidate) {
  // Compute the benefit of this candidate.

  // Merge per-object information.
  Map<llvm::Function *, Set<llvm::Value *>> encoded;
  Map<llvm::Function *, Set<llvm::Use *>> to_encode, to_addkey;
  for (const auto *obj : candidate)
    for (const auto &[func, info] : obj->info()) {
      encoded[func].insert(info.encoded.begin(), info.encoded.end());
      to_encode[func].insert(info.to_encode.begin(), info.to_encode.end());
      to_addkey[func].insert(info.to_addkey.begin(), info.to_addkey.end());
    }

  // Perform a forward data flow analysis on the encoded values.
  forward_analysis(encoded);

  // TODO: Gather the set of uses to decode based on the forward analysis, as
  // done in Optimize.cpp, then update heuristic to use uses  instead of encoded
  // values. Otherwise, the cost is unfairly counted.

  // Collect the set of uses to decode.
  Map<llvm::Function *, Set<llvm::Use *>> to_decode;
  for (const auto &[func, values] : encoded) {
    for (auto *val : values) {
      for (auto &use : val->uses()) {
        auto *user = use.getUser();

        // If the user is a PHI/Select/Fold _and_ is also encoded, we don't
        // need to decode it.
        if (isa<llvm::PHINode>(user) or isa<llvm::SelectInst>(user)) {
          if (values.contains(user))
            continue;

        } else if (auto *fold = into<FoldInst>(user)) {
          auto *arg = (&use == &fold->getInitialAsUse())
                          ? &fold->getAccumulatorArgument()
                          : fold->getClosedArgument(use);
          if (encoded[&fold->getBody()].count(arg))
            continue;

        } else if (auto *ret = dyn_cast<llvm::ReturnInst>(user)) {
          if (auto *func = ret->getFunction()) {
            if (auto *fold = FoldInst::get_single_fold(*func)) {
              auto *caller = fold->getFunction();
              auto &result = fold->getResult();
              // If the result of the fold is encoded, we don't decode here.
              if (encoded[caller].count(&result))
                continue;
            }
          }
        } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
          // TODO
        }

        // Otherwise, mark the use for decoding.
        to_decode[func].insert(&use);
      }
    }
  }

#if 0
  debugln("== USES ==");
  debugln("TO DECODE");
  print_uses(to_decode);
  debugln("TO ENCODE");
  print_uses(to_encode);
  debugln("TO ADDKEY");
  print_uses(to_addkey);
  debugln();
#endif

  // Perform a "what if?" analysis.
  return heuristic(to_decode, to_encode, to_addkey);
}

} // namespace memoir
