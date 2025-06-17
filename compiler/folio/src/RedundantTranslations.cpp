#include "llvm/Support/CommandLine.h"

#include "folio/RedundantTranslations.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::cl::opt<bool> disable_translation_elimination(
    "disable-translation-elimination",
    llvm::cl::desc("Disable redundant translation elimination"),
    llvm::cl::init(false));

static bool used_value_will_be_decoded(
    llvm::Use &use,
    llvm::Value *base,
    const LocalMap<Set<llvm::Use *>> &base_to_decode,
    Set<llvm::Use *> &visited) {

  debugln("DECODED? ", *use.get());

  if (not base_to_decode.contains(base)) {
    return false;
  }
  const auto &to_decode = base_to_decode.lookup(base);

  if (to_decode.count(&use) > 0) {
    debugln("  YES");
    return true;
  }

  if (visited.count(&use) > 0) {
    debugln("  YES");
    return true;
  } else {
    visited.insert(&use);
  }

  auto *value = use.get();
  if (auto *phi = dyn_cast<llvm::PHINode>(value)) {
    debugln("  RECURSE");
    bool all_decoded = true;
    for (auto &incoming : phi->incoming_values()) {
      all_decoded &=
          used_value_will_be_decoded(incoming, base, base_to_decode, visited);
    }
    return all_decoded;
  } else if (auto *select = dyn_cast<llvm::SelectInst>(value)) {
    debugln("  RECURSE");
    return used_value_will_be_decoded(select->getOperandUse(1),
                                      base,
                                      base_to_decode,
                                      visited)
           and used_value_will_be_decoded(select->getOperandUse(2),
                                          base,
                                          base_to_decode,
                                          visited);
  } else if (auto *arg = dyn_cast<llvm::Argument>(value)) {
    auto &func =
        MEMOIR_SANITIZE(arg->getParent(), "Argument has no parent function!");
    if (auto *fold = FoldInst::get_single_fold(func)) {
      if (auto *operand_use = fold->getOperandForArgument(*arg)) {
        debugln("  RECURSE");
        return used_value_will_be_decoded(*operand_use,
                                          base,
                                          base_to_decode,
                                          visited);
      }
    }
  }

  debugln("  NO");
  return false;
}

static bool used_value_will_be_decoded(
    llvm::Use &use,
    llvm::Value *base,
    const LocalMap<Set<llvm::Use *>> &to_decode) {
  Set<llvm::Use *> visited = {};
  return used_value_will_be_decoded(use, base, to_decode, visited);
}

void eliminate_redundant_translations(LocalMap<Set<llvm::Use *>> &to_decode,
                                      LocalMap<Set<llvm::Use *>> &to_encode,
                                      LocalMap<Set<llvm::Use *>> &to_addkey) {

  if (not disable_translation_elimination) {

    // TODO: Perform a data flow analysis ahead of time to determine which
    // values will be decoded at the use site, and with which base.

    // Trim uses that dont need to be decoded because they are only used to
    // compare against other values that need to be decoded.
    Set<llvm::Use *> trim_to_decode = {};
    for (const auto &[base, local_uses] : to_decode) {
      for (auto *use : local_uses) {
        auto *user = use->getUser();

        if (auto *cmp = dyn_cast<llvm::CmpInst>(user)) {
          if (cmp->isEquality()) {
            auto &lhs = cmp->getOperandUse(0);
            auto &rhs = cmp->getOperandUse(1);
            if (used_value_will_be_decoded(lhs, base, to_decode)
                and used_value_will_be_decoded(rhs, base, to_decode)) {
              trim_to_decode.insert(&lhs);
              trim_to_decode.insert(&rhs);
            }
          }
        }
      }
    }

    // Trim uses that dont need to be encoded because they are produced by a
    // use that needs decoded.
    Set<llvm::Use *> trim_to_encode = {};
    for (auto uses : { to_encode, to_addkey }) {
      for (const auto &[base, local_uses] : uses) {
        for (auto *use : local_uses) {
          if (used_value_will_be_decoded(*use,
                                         base,
                                         to_decode)) { // TODO: add base.
            trim_to_encode.insert(use);
            trim_to_decode.insert(use);
          }
        }
      }
    }

    // Erase the uses that we identified to trim.
    erase_uses(to_decode, trim_to_decode);
    erase_uses(to_encode, trim_to_encode);
    erase_uses(to_addkey, trim_to_encode);
  }
}

} // namespace folio
