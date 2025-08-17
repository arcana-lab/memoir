#include "memoir/ir/CallGraph.hpp"
#include "memoir/support/UnionFind.hpp"
#include "memoir/support/WorkList.hpp"

#include "folio/Candidate.hpp"
#include "folio/ProxyInsertion.hpp"
#include "folio/RedundantTranslations.hpp"
#include "folio/Utilities.hpp"
#include "folio/WeakenUses.hpp"

using namespace llvm::memoir;

namespace folio {

void Candidate::gather_uses(
    const Map<ObjectInfo *, SmallVector<ObjectInfo *>> &equiv) {

  // Gather uses and encoded values within each equivalence class.
  for (auto *base : *this)
    if (equiv.contains(base))
      for (auto *obj : equiv.at(base))
        for (const auto &[func, local] : obj->info()) {
          this->encoded[obj][func].insert(local.encoded.begin(),
                                          local.encoded.end());
          this->to_encode[obj][func].insert(local.to_encode.begin(),
                                            local.to_encode.end());
          this->to_addkey[obj][func].insert(local.to_addkey.begin(),
                                            local.to_addkey.end());
        }

  // Perform a forward analysis.
  for (auto &[base, values] : this->encoded)
    forward_analysis(values);

  // Debug print.
  for (const auto &[base, locals] : this->encoded) {
    for (const auto &[func, values] : locals)
      for (auto *val : values) {
        print("  ENCODED ", *val);
        if (auto *arg = dyn_cast<llvm::Argument>(val))
          println(" IN ", arg->getParent()->getName());
        else
          println();
      }
  }

  // Collect the set of uses to decode.
  println("=== COLLECT USES TO DECODE ===");
  for (auto &[base, equiv_encoded] : this->encoded)
    for (const auto &[func, values] : equiv_encoded) {
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
            if (equiv_encoded[&fold->getBody()].count(arg))
              continue;

          } else if (auto *ret = dyn_cast<llvm::ReturnInst>(user)) {
            if (auto *func = ret->getFunction()) {
              if (auto *fold = FoldInst::get_single_fold(*func)) {
                auto *caller = fold->getFunction();
                auto &result = fold->getResult();
                // If the result of the fold is encoded, we don't decode here.
                if (equiv_encoded[caller].count(&result))
                  continue;
              }
            }
          } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
            // TODO
          }

          // Otherwise, mark the use for decoding.
          this->to_decode[base][func].insert(&use);
        }
      }
    }
}

void ProxyInsertion::optimize() {

  // Optimize the uses in each candidate.
  for (auto &candidate : this->candidates) {
    println("OPTIMIZE");
    println(candidate);

    // Collect all of the uses that need to be handled.
    candidate.gather_uses(this->equiv);

    println("  FOUND USES");
    println("    TO ENCODE");
    print_uses(candidate.to_encode);
    println("    TO DECODE");
    print_uses(candidate.to_decode);
    println("    TO ADDKEY");
    print_uses(candidate.to_addkey);

#if 0
  if (not disable_use_weakening) {
    // DISABLED: Use weakening works, but does not have any considerable
    // performance benefits.

    // Weaken uses from addkey to encode if we know that the value is already
    // inserted.
    Set<llvm::Use *> to_weaken = {};
    weaken_uses(to_addkey, to_weaken, candidate, get_bounds_checks);

    erase_uses(to_addkey, to_weaken);
    for (auto *use : to_weaken) {
      to_encode.insert(use);
    }
  }
#endif

    for (const auto &[base, encoded] : candidate.encoded)
      eliminate_redundant_translations(encoded,
                                       candidate.to_decode[base],
                                       candidate.to_encode[base],
                                       candidate.to_addkey[base]);

    println("  TRIMMED USES:");
    println("    TO ENCODE");
    print_uses(candidate.to_encode);
    println("    TO DECODE");
    print_uses(candidate.to_decode);
    println("    TO ADDKEY");
    print_uses(candidate.to_addkey);
  }
}

} // namespace folio
