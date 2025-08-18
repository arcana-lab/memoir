#include "llvm/Support/CommandLine.h"

#include "folio/RedundantTranslations.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::cl::opt<bool> disable_translation_elimination(
    "disable-translation-elimination",
    llvm::cl::desc("Disable redundant translation elimination"),
    llvm::cl::init(false));

void eliminate_redundant_translations(
    const Map<llvm::Function *, Set<llvm::Value *>> &encoded,
    Map<llvm::Function *, Set<llvm::Use *>> &to_decode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_encode,
    Map<llvm::Function *, Set<llvm::Use *>> &to_addkey) {

  if (disable_translation_elimination)
    return;

  println(" >> ELIMINATE REDUNDANT TRANSLATIONS << ");

  // Trim uses that dont need to be decoded because they are only used to
  // compare against other values that need to be decoded.
  Set<llvm::Use *> trim_to_decode = {};
  for (const auto &[func, local_uses] : to_decode) {
    if (not encoded.contains(func))
      continue;
    const auto &local_encoded = encoded.at(func);
    for (auto *use : local_uses) {
      if (auto *cmp = dyn_cast<llvm::CmpInst>(use->getUser())) {
        if (cmp->isEquality()) {
          auto &lhs = cmp->getOperandUse(0);
          auto &rhs = cmp->getOperandUse(1);
          if (local_encoded.contains(lhs.get())
              and local_encoded.contains(rhs.get())) {
            println("TRIM ", pretty_use(*use));
            trim_to_decode.insert(&lhs);
            trim_to_decode.insert(&rhs);
          }
        }
      }
    }
  }

  // Trim uses that dont need to be encoded because they use an already
  // encoded value.
  Set<llvm::Use *> trim_to_encode = {};
  for (const auto &uses : { to_encode, to_addkey }) {
    for (const auto &[func, local_uses] : uses) {
      if (not encoded.contains(func))
        continue;
      const auto &local_encoded = encoded.at(func);
      for (auto *use : local_uses) {
        println("TRIM? ", pretty_use(*use));
        if (local_encoded.contains(use->get())) {
          println("  YES");
          trim_to_encode.insert(use);
          trim_to_decode.insert(use);
        } else {
          println("  NO");
        }
      }
    }
  }

  // Erase the uses that we identified to trim.
  erase_uses(to_decode, trim_to_decode);
  erase_uses(to_encode, trim_to_encode);
  erase_uses(to_addkey, trim_to_encode);
}

} // namespace folio
