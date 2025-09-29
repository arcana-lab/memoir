#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/WorkList.hpp"

namespace memoir {

/**
 * Find the PHI node that uses the value, if one exists.
 */
static llvm::PHINode *find_phi_using(llvm::BasicBlock &predecessor,
                                     llvm::BasicBlock &successor,
                                     llvm::Value &used) {
  // For each PHI in the successor.
  for (auto &phi : successor.phis()) {

    // Fetch the value used on the basic block edge from the predecessor.
    auto *value = phi.getIncomingValueForBlock(&predecessor);

    // Check against the value we're searching for.
    if (value == &used) {
      return &phi;
    }
  }

  return NULL;
}

static void add_control_flow_facts(BoundsCheckResult &result,
                                   llvm::BasicBlock &BB) {
  // Ensure that the terminator is a branch.
  auto *branch = dyn_cast<llvm::BranchInst>(BB.getTerminator());
  if (not branch) {
    return;
  }

  // Ensure that it is a conditional branch.
  if (not branch->isConditional()) {
    return;
  }

  // Fetch the condition.
  auto *cond = branch->getCondition();

  // If the branch is conditioned on a has instruction, handle it.
  if (auto *has = into<HasInst>(cond)) {
    // Fetch the used object.
    auto &object = has->getObject();

    // Attach the fact to the PHI on the then branch.
    auto &then_block =
        MEMOIR_SANITIZE(branch->getSuccessor(0), "Then block is NULL!");
    if (auto *then_phi = find_phi_using(BB, then_block, object)) {
      result.initialize(*then_phi, BoundsCheck(*has));
    }

    // Attach the negated fact to the PHI on the else branch.
    auto &else_block =
        MEMOIR_SANITIZE(branch->getSuccessor(1), "Else block is NULL!");
    if (auto *else_phi = find_phi_using(BB, else_block, object)) {
      result.initialize(*else_phi, BoundsCheck(*has, true));
    }
  }

  return;
}

static void add_data_flow_facts(BoundsCheckResult &result,
                                llvm::BasicBlock &BB) {

  for (auto &I : BB) {
    if (auto *alloc = into<AllocInst>(I)) {
      result.initialize(I);
    }

    // TODO: we could perform something akin to LLVM's null-pointer analysis,
    // where accessing a key would mean that it is valid.
    // This would mean that all nested indices introduce facts, and all
  }

  return;
}

static bool transfer(BoundsCheckResult &result, llvm::Use &use) {

  auto *user = dyn_cast_or_null<llvm::Instruction>(use.getUser());
  if (not user) {
    return false;
  }

  if (auto *update = into<UpdateInst>(user)) {

    // Ensure that the use is the object being updated.
    if (&use != &update->getObjectAsUse()) {
      return false;
    }

    // Fetch the checks for this instruction.
    auto &checks = result[*user];

    // Handle instructions that insert/remove keys.
    if (auto *insert = dyn_cast<InsertInst>(update)) {
      // ---
      // out = insert(in, k...) => facts(out) = facts(in) U { [k...] }
      return checks.join(result[insert->getObject()])
             or checks.insert(BoundsCheck(*insert));

    } else if (auto *remove = dyn_cast<RemoveInst>(update)) {
      // ---
      // out = remove(in, k...) => facts(out) = { ¬[k...] }
      return checks.insert(BoundsCheck(*remove, /* negated? */ true));

      // TODO: Make this a bit smarter by using some form of value numbering to
      // maintain facts that _definitely don't_ conflict with the remove.

    } else if (auto *clear = dyn_cast<ClearInst>(update)) {
      // ---
      // out = clear(in, k...) => facts(out) = {}
      if (not result.contains(*user)) {
        result.initialize(*user);
        return true;
      }

      // TODO: Make this a bit smarter by using some form of value numbering to
      // maintain facts that _definitely don't_ conflict with the clear, if it
      // is on a nested collection.

    } else {
      // update != insert ^ update != remove
      // ---
      // out = update(in, ...) => facts(out) = facts(in)
      return checks.join(result[update->getObject()]);
    }
  } else if (auto *fold = into<FoldInst>(user)) {
    // Ensure that the use is the accumulator.
    if (&use != &fold->getInitialAsUse()) {
      return false;
    }

    // ---
    // out = fold(func, in, over, k...) => facts(out) = {}
    if (not result.contains(*user)) {
      result.initialize(*user);
      return true;
    }
  } else if (auto *ret_phi = into<RetPHIInst>(user)) {
    // ---
    // out = retphi(in) => facts(out) = {}
    if (not result.contains(*user)) {
      result.initialize(*user);
      return true;
    }
  } else if (auto *phi = dyn_cast<llvm::PHINode>(user)) {

    bool changed = false;
    if (not result.contains(*phi)) {
      auto &checks = result[*phi];

      // Copy the first incoming value.
      auto *first = phi->getIncomingValue(0);
      changed |= checks.join(result[*first]);
    }

    // ---
    // out = phi(a, b) => facts(out) = facts(a) ∩ facts(b)

    // TODO: the following code can be simplified by only updating the use being
    // propagated.

    auto &checks = result[*phi];
    for (auto &incoming : phi->incoming_values()) {
      auto &value = MEMOIR_SANITIZE(incoming.get(),
                                    "Incoming value for PHI is NULL!\n",
                                    *phi);
      if (result.contains(value)) {
        changed |= checks.meet(result[value]);
      }
    }
    return changed;
  }

  return false;
}

BoundsCheckResult BoundsCheckAnalysis::run(llvm::Function &F,
                                           llvm::FunctionAnalysisManager &FAM) {
  BoundsCheckResult result;

  // Construct the initial analysis result.

  // Construct an empty set of checks for each argument.
  for (auto &A : F) {
    // If the argument is an object, create an empty set for it.
    if (Type::value_is_object(A)) {
      result.initialize(A);
    }
  }

  // Initialize the checks set for operations that add/remove checks.
  for (auto &BB : F) {
    add_control_flow_facts(result, BB);

    add_data_flow_facts(result, BB);
  }

  // If we found no initial facts, return early.
  if (result.empty()) {
    return result;
  }

  debugln("[Bounds Check] Running on ", F.getName());
  debugln("[Bounds Check] Initial Facts:");
  debugln(result);
  debugln("==========");

  // Collect the initial worklist.
  WorkList<llvm::Value *> worklist;
  for (const auto &[val, checks] : result) {
    worklist.push(val);
  }

  // Apply the transfer function until a fixed point is reached.
  while (not worklist.empty()) {

    // Process a new value from the worklist.
    auto *val = worklist.pop();

    for (auto &use : val->uses()) {
      if (transfer(result, use)) {
        worklist.push(use.getUser());
      }
    }
  }

  debugln("[Bounds Check] Result:");
  debugln(result);
  debugln("==========");

  return result;
}

llvm::AnalysisKey BoundsCheckAnalysis::Key;

} // namespace memoir
