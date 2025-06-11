#include "llvm/IR/Function.h"

#include "folio/transforms/CoalesceUses.hpp"
#include "folio/transforms/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

// Command line options.
static llvm::cl::opt<bool> disable_use_coalescing(
    "disable-proxy-use-coalescing",
    llvm::cl::desc("Disable coalescing proxy uses"),
    llvm::cl::init(false));

// CoalescedUses implementation.
llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const CoalescedUses &uses) {

  auto &value = uses.value();

  os << value;

  if (auto *func = parent_function(value)) {
    os << " IN " << func->getName();
  }

  os << "\n";

  for (auto *use : uses) {
    os << "  OP" << use->getOperandNo() << " IN " << *use->getUser() << "\n";
  }

  return os;
}

using GroupedUses =
    Map<llvm::Function *, Map<llvm::Value *, Vector<llvm::Use *>>>;
static GroupedUses groupby_function_and_used(const Set<llvm::Use *> &uses) {

  GroupedUses local;

  for (auto *use : uses) {
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());
    if (not user) {
      warnln("Non-instruction user found during ProxyInsertion, unexpected.");
      continue;
    }

    auto *func = user->getFunction();

    auto *used = use->get();

    local[func][used].push_back(use);
  }

  return local;
}

static void sort_in_level_order(Vector<llvm::Use *> &uses,
                                llvm::DominatorTree &DT) {

  // First, sort the uses in level order of the dominator tree.
  std::sort(uses.begin(), uses.end(), [&DT](llvm::Use *lhs, llvm::Use *rhs) {
    // Get the user instructions.
    auto *lhs_inst = cast<llvm::Instruction>(lhs->getUser());
    auto *rhs_inst = cast<llvm::Instruction>(rhs->getUser());

    // Get the parent basic blocks.
    auto *lhs_block = lhs_inst->getParent();
    auto *rhs_block = rhs_inst->getParent();

    // Partial domtree level order between basic block.
    auto *lhs_node = DT[lhs_block];
    auto *rhs_node = DT[rhs_block];

    MEMOIR_ASSERT(lhs_node, "LHS NODE = NULL");
    MEMOIR_ASSERT(rhs_node, "RHS NODE = NULL");

    auto lhs_level = lhs_node->getLevel();
    auto rhs_level = rhs_node->getLevel();

    if (lhs_level < rhs_level) {
      return true;
    } else if (rhs_level < lhs_level) {
      return false;
    }

    if (lhs_block == rhs_block) {
      return lhs_inst->comesBefore(rhs_inst);
    }

    return lhs_block < rhs_block;
  });

  return;
}

static void coalesce_by_dominance(
    Vector<CoalescedUses> &coalesced,
    GroupedUses &grouped,
    std::function<llvm::DominatorTree &(llvm::Function &)> get_dominator_tree) {

  for (auto &[func, locals] : grouped) {

    // Fetch the dominators for this function.
    auto &DT = get_dominator_tree(*func);

    // For each of the local values being decoded:
    for (auto &[val, uses] : locals) {

      // Special handling for values with single uses.
      if (uses.size() == 1) {
        coalesced.emplace_back(uses);
        continue;
      }

      // Sort the uses in level order of the dominator tree.
      sort_in_level_order(uses, DT);

      // Group together uses that are dominated by one another.
      Set<llvm::Use *> visited = {};
      for (auto it = uses.begin(); it != uses.end(); ++it) {
        auto *use = *it;

        if (visited.count(use) > 0) {
          continue;
        } else {
          visited.insert(use);
        }

        // Unpack the use.
        auto *user = cast<llvm::Instruction>(use->getUser());

        // Add a new coalesced use.
        coalesced.emplace_back(use);
        auto &current = coalesced.back();

        // If coalescing is disabled, then don't!
        if (disable_use_coalescing) {
          continue;
        }

        // Don't coalesce has operations.
        if (into<HasInst>(user)) {
          continue;
        }

        // Try to coalesce the remaining uses.
        for (auto it2 = std::next(it); it2 != uses.end(); ++it2) {
          auto *other_use = *it2;
          auto *other_user = cast<llvm::Instruction>(other_use->getUser());

          if (DT.dominates(user, *other_use)) {
            current.push_back(other_use);
            visited.insert(other_use);

          } else if (DT.dominates(other_user, *use)) {
            // This check is unnecessary, it's here as a sanity check.
            MEMOIR_UNREACHABLE("Level order is incorrect!\n",
                               "      ",
                               *other_user,
                               " doms ",
                               *user);
          }
        }
      }
    }
  }

  for (auto &uses : coalesced) {
    debugln("COALESCED ", uses);
  }

  return;
}

void coalesce(
    Vector<CoalescedUses> &coalesced,
    const Set<llvm::Use *> &uses,
    std::function<llvm::DominatorTree &(llvm::Function &)> get_dominator_tree) {
  auto grouped = groupby_function_and_used(uses);

  coalesce_by_dominance(coalesced, grouped, get_dominator_tree);
}

} // namespace folio
