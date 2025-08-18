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

static void gather_uses(llvm::ArrayRef<ObjectInfo *> objects,
                        ProxyInsertion::TransformInfo &info) {

  // Gather uses and encoded values within each equivalence class.
  for (auto *obj : objects)
    for (const auto &[func, local] : obj->info()) {
      info.encoded[func].insert(local.encoded.begin(), local.encoded.end());
      info.to_encode[func].insert(local.to_encode.begin(),
                                  local.to_encode.end());
      info.to_addkey[func].insert(local.to_addkey.begin(),
                                  local.to_addkey.end());
    }

  // Perform a forward analysis.
  forward_analysis(info.encoded);

  // Debug print.
  println("=== ENCODED VALUES ===");
  for (const auto &[func, values] : info.encoded) {
    for (auto *val : values) {
      print("  ENCODED ", pretty(*val));
      if (auto *arg = dyn_cast<llvm::Argument>(val))
        println(" IN ", arg->getParent()->getName());
      else
        println();
    }
  }
  println();

  // Collect the set of uses to decode.
  println("=== COLLECT USES TO DECODE ===");
  for (const auto &[func, values] : info.encoded) {
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
          if (info.encoded[&fold->getBody()].count(arg))
            continue;

        } else if (auto *ret = dyn_cast<llvm::ReturnInst>(user)) {
          if (auto *func = ret->getFunction()) {
            if (auto *fold = FoldInst::get_single_fold(*func)) {
              auto *caller = fold->getFunction();
              auto &result = fold->getResult();
              // If the result of the fold is encoded, we don't decode here.
              if (info.encoded[caller].count(&result))
                continue;
            }
          }
        } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
          // TODO
        }

        // Otherwise, mark the use for decoding.
        info.to_decode[func].insert(&use);
      }
    }
  }
}

void ProxyInsertion::optimize() {

  // Optimize the uses in each equivalence class.
  for (const auto &[base, objects] : this->equiv) {
    { // Debug print.
      println("OPTIMIZE CLASS IN ", base->function()->getName());
      for (auto *obj : objects)
        println("  ", *obj);
      println();
    }

    // Gather info from each object together.
    auto &info = this->to_transform[base];
    gather_uses(objects, info);

    // Optimize the set of uses.
    eliminate_redundant_translations(info.encoded,
                                     info.to_decode,
                                     info.to_encode,
                                     info.to_addkey);
    { println(); }
  }
}

} // namespace folio
