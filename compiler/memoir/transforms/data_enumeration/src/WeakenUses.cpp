#if 0
#  include "llvm/IR/CFG.h"

#  include "memoir/analysis/BoundsCheckAnalysis.hpp"
#  include "memoir/support/Assert.hpp"
#  include "memoir/support/Casting.hpp"
#  include "memoir/support/Print.hpp"
#  include "memoir/support/SortedVector.hpp"
#  include "memoir/support/WorkList.hpp"

#  include "memoir/transforms/data_enumeration/DataEnumeration.hpp"
#  include "memoir/transforms/data_enumeration/Utilities.hpp"

using namespace memoir;

namespace memoir {

static void propagate_data_flow(
    Map<llvm::Instruction *, SortedVector<llvm::Value *>> &facts) {

  // Initialize a worklist.
  WorkList<llvm::Instruction *> worklist;
  for (const auto &[inst, list] : facts) {
    worklist.push(inst);
  }

  while (not worklist.empty()) {

    // Pop an item off the worklist.
    auto *inst = worklist.pop();

    // Collect the set of successors.
    if (auto *branch = dyn_cast<llvm::BranchInst>(inst)) {
      for (auto *succ_block : branch->successors()) {
        auto *succ = &succ_block->front();

        auto old_size = facts[succ].size();

        facts[succ].set_union(facts[inst]);

        for (auto *pred_block : llvm::predecessors(succ_block)) {
          auto *pred = &pred_block->back();

          if (pred != inst) {
            facts[succ].set_intersection(facts[pred]);
          }
        }

        if (facts[succ].size() != old_size) {
          worklist.push(succ);
        }
      }
    } else if (auto *succ = inst->getNextNode()) {

      auto &succ_facts = facts[succ];

      if (succ_facts.set_union(facts[inst])) {
        worklist.push(succ);
      }

      // Handle trivial copies.
      if (auto *phi = dyn_cast<llvm::PHINode>(inst)) {
        if (auto *val = phi->hasConstantValue()) {
          if (succ_facts.find(val) != succ_facts.end()) {
            if (succ_facts.insert(phi)) {
              worklist.push(succ);
            }
          }
        }
      }
    }
  }
}

void weaken_uses(Set<llvm::Use *> &to_addkey,
                 Set<llvm::Use *> &to_weaken,
                 Candidate &candidate,
                 DataEnumeration::GetBoundsChecks get_bound_checks) {

  // Collect all of the functions where addkey uses occur.
  Map<llvm::Function *, Set<llvm::Use *>> local_uses = {};
  for (auto *use : to_addkey) {
    auto &user = MEMOIR_SANITIZE(use->getUser(), "Use has NULL user!");
    local_uses[parent_function(user)].insert(use);
  }

  // Create a reverse mapping from redefinition to ObjectInfo(s).
  Map<llvm::Value *, Set<ObjectInfo *>> redef_to_infos = {};
  for (auto *info : candidate) {
    for (const auto &[func, base_to_redefs] : info->redefinitions) {
      for (const auto &[base, redefs] : base_to_redefs.second) {
        for (const auto &redef : redefs) {
          if (local_uses.count(func)) {
            redef_to_infos[&redef.value()].insert(info);
          }
        }
      }
    }
  }

  // Iterate over each of the functions with uses.
  for (const auto &[func, uses] : local_uses) {
    // Fetch the bound checks for this function.
    auto &bound_checks = get_bound_checks(*func);

    // For each object in the candidate, collate the sparse results into a
    // single dense result.
    Map<llvm::Instruction *, SortedVector<llvm::Value *>> present = {};

    // Initialize the mapping with the analysis results.
    for (const auto &[val, checks] : bound_checks) {

      // Ensure that the value being checked is in the candidate.
      if (not redef_to_infos.count(val)) {
        continue;
      }

      // Fetch the set of infos.
      auto &infos = redef_to_infos[val];

      // Only handle instructions.
      auto *inst = dyn_cast<llvm::Instruction>(val);
      if (not inst) {
        continue;
      }

      debugln("INST: ", *inst);

      for (const auto &check : checks) {

        debugln("  CHECK: ", check);

        // Skip negated checks.
        if (check.negated()) {
          debugln("    NEGATED!");
          continue;
        }

        // If the check indices match an info offset, insert the key.
        for (auto *info : infos) {

          debugln("    INFO: ", *info);

          // If the check is at the relevant depth for this info, insert it.
          if (check.indices().size() == (info->offsets.size() + 1)) {
            debugln("      YES!");
            auto *next = inst->getNextNode();
            present[next].insert(&check.key());
          }
        }
      }
    }

    // Propagate present key information.
    propagate_data_flow(present);

    // Debug print the data flow results.
    debugln("PRESENT KEYS IN ", func->getName());
    for (auto &BB : *func) {
      debugln(value_name(BB), ":");
      for (auto &I : BB) {
        if (present.count(&I)) {
          debug("  ", I /*value_name(I)*/, " ⊢ ");
          if (present[&I].empty()) {
            debugln(Style::BOLD, Colors::RED, "∅", Style::RESET);

          } else {
            debug(Style::BOLD, Colors::GREEN, "{ ");
            for (const auto *key : present[&I]) {
              debug(value_name(*key), " ");
            }
            debugln("}", Style::RESET);
          }
        }
      }
    }

    // For each use in this function, if it uses a function that is already
    // present in the proxy, weaken it.
    for (auto *use : uses) {
      // Ensure that the user is an instruction.
      auto *inst = dyn_cast<llvm::Instruction>(use->getUser());
      if (not inst) {
        continue;
      }

      // Skip instructions that have no present information.
      if (not present.count(inst)) {
        continue;
      }
      auto &keys = present[inst];

      // If the value used is present, weaken the use.
      auto *used = use->get();
      if (keys.find(used) != keys.end()) {
        to_weaken.insert(use);
        debugln(Style::BOLD, "WEAKEN ", pretty_use(*use), Style::RESET);
      }
    }
  }

  return;
}

} // namespace memoir
#endif
