#include <functional>

#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include "memoir/ir/Types.hpp"
#include "memoir/raise/RepairSSA.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/DataTypes.hpp"

namespace memoir {

void repair_ssa(llvm::ArrayRef<llvm::AllocaInst *> vars,
                llvm::DominatorTree &domtree) {
  // Gather the variables that can be promoted.
  Vector<llvm::AllocaInst *> to_promote = {};
  for (auto *var : vars) {
    if (llvm::isAllocaPromotable(var)) {
      to_promote.push_back(var);
    } else {
      warnln("Cannot promote alloca: ", *var);
    }
  }

  // Promote them.
  llvm::PromoteMemToReg(to_promote, domtree);
}

void repair_ssa(
    llvm::ArrayRef<llvm::AllocaInst *> vars,
    std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree) {

  // Group the variables by their parent function, ensuring that each can be
  // promoted.
  Map<llvm::Function *, Vector<llvm::AllocaInst *>> locals;
  for (auto *var : vars) {
    MEMOIR_ASSERT(var, "Stack variables is NULL!");

    auto *func = var->getFunction();
    MEMOIR_ASSERT(func, "Stack variable has no parent function!");

    locals[func].push_back(var);
  }

  // For each group of stack variables, promote them to registers.
  for (const auto &[func, to_promote] : locals) {
    repair_ssa(to_promote, get_domtree(*func));
  }
}

void repair_ssa(llvm::Function &func, llvm::DominatorTree &domtree) {

  // Gather all of the alloca instructions in the function.
  Vector<llvm::AllocaInst *> vars = {};
  for (auto &bb : func) {
    for (auto &inst : bb) {
      auto *var = dyn_cast<llvm::AllocaInst>(&inst);
      if (not var) {
        continue;
      }

      // Ensure that the stack variable stores a pointer.
      if (not isa<llvm::PointerType>(var->getAllocatedType())) {
        continue;
      }

      // Check that there exists a load/store from this pointer that is an
      // object.
      bool is_object = false;
      for (auto &use : var->uses()) {
        auto *user = use.getUser();

        // Fetch the MEMOIR type of the value loaded/stored, if it exists.
        Type *type = NULL;
        if (auto *load = dyn_cast<llvm::LoadInst>(user)) {
          if (var == load->getPointerOperand()) {
            is_object |= Type::value_is_object(*load);
          }

        } else if (auto *store = dyn_cast<llvm::StoreInst>(user)) {
          if (var == store->getPointerOperand()) {
            auto &stored = MEMOIR_SANITIZE(store->getValueOperand(),
                                           "Stored value is NULL!");

            is_object |= Type::value_is_object(stored);
          }
        }

        // If we found an object, we can stop searching early.
        if (is_object) {
          break;
        }
      }

      // If this variable stores an object, add it to the repair list.
      if (is_object) {
        vars.push_back(var);
      }
    }
  }

  // Repair the allocas.
  repair_ssa(vars,
             [&domtree](llvm::Function &other_func) -> llvm::DominatorTree & {
               return domtree;
             });

  return;
}

} // namespace memoir
