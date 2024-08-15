#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "folio/analysis/ConstraintInference.hpp"

#include "folio/passes/Pass.hpp"

using namespace llvm::memoir;

namespace folio {

namespace detail {
uint32_t get_id(map<llvm::Value *, uint32_t> &value_ids,
                uint32_t &current_id,
                llvm::Value &V) {
  auto found = value_ids.find(&V);
  if (found != value_ids.end()) {
    return found->second;
  }

  // If we couldn't find an ID, create one.
  auto id = current_id++;
  value_ids[&V] = id;

  return id;
}
} // namespace detail

llvm::PreservedAnalyses FolioPass::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {

  // Fetch the ConstraintInference results.
  auto &constraints = MAM.getResult<ConstraintInference>(M);

  // Bookkeeping.
  uint32_t current_id = 0;
  map<llvm::Value *, uint32_t> value_ids;

  // Collect all of the selectable variables.
  set<llvm::Value *> selectable = {};
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *memoir_inst = into<MemOIRInst>(&I);
        if (not memoir_inst) {
          continue;
        }

        // For the time being, we will only consider explicit allocations as
        // selectable.
        if (auto *seq = dyn_cast<SequenceAllocInst>(memoir_inst)) {
          auto id = detail::get_id(value_ids, current_id, I);

          println(I);
          print("  :- ");

          print("seq(", id, ").");

          // Print the constraints.
          for (auto constraint : constraints[I]) {
            print(" ", constraint.name(), "(", std::to_string(id), ").");
          }
          println();

          selectable.insert(&I);
        } else if (auto *assoc = dyn_cast<AssocAllocInst>(memoir_inst)) {
          auto id = detail::get_id(value_ids, current_id, I);

          println(I);
          print("  :- ");
          print("assoc(", id, ").");

          // Print the constraints.
          for (auto constraint : constraints[I]) {
            print(" ", constraint.name(), "(", std::to_string(id), ").");
          }
          println();

          selectable.insert(&I);
        }
      }
    }
  }

  // Print the constraint results.
  // for (const auto &[value, constraint_set] : constraints) {

  //   println(*value);
  //   print("  :- ");
  //   auto id = detail::get_id(value_ids, current_id, *value);

  //   for (const auto constraint : constraint_set) {
  //     print(constraint.name(), "(", std::to_string(id), "). ");
  //   }
  //   println();
  // }

  // All done.
  return llvm::PreservedAnalyses::none();
}

} // namespace folio
