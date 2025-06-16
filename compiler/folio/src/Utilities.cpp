#include "llvm/IR/Instructions.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/WorkList.hpp"

#include "folio/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

bool is_last_index(llvm::Use *use, AccessInst::index_op_iterator index_end) {
  return std::next(AccessInst::index_op_iterator(use)) == index_end;
}

llvm::Function *parent_function(llvm::Value &V) {
  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return arg->getParent();
  } else if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return inst->getFunction();
  }
  return nullptr;
}

uint32_t forward_analysis(Map<llvm::Function *, Set<llvm::Value *>> &encoded) {

  uint32_t count = 0;

  WorkList<llvm::Value *> worklist;

  for (auto &[func, values] : encoded) {
    worklist.push(values.begin(), values.end());
  }

  while (not worklist.empty()) {
    auto *val = worklist.pop();

    auto *func = parent_function(*val);
    auto &local_encoded = encoded[func];

    for (auto &use : val->uses()) {
      auto *user = use.getUser();

      if (local_encoded.count(user)) {
        continue;
      }

      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        bool all_encoded = true;
        for (auto &incoming : phi->incoming_values()) {
          if (not local_encoded.count(incoming.get())) {
            all_encoded = false;
            break;
          }
        }

        if (all_encoded) {
          local_encoded.insert(phi);
          worklist.push(phi);
          ++count;
        }

      } else if (auto *select = dyn_cast<llvm::SelectInst>(user)) {
        auto *lhs = select->getTrueValue();
        auto *rhs = select->getFalseValue();

        if (local_encoded.count(lhs) and local_encoded.count(rhs)) {
          local_encoded.insert(select);
          worklist.push(select);
          ++count;
        }
      } else if (auto *ret = dyn_cast<llvm::ReturnInst>(user)) {
        auto *func = ret->getFunction();
        if (not func) {
          continue;
        }

        if (auto *fold = FoldInst::get_single_fold(*func)) {
          auto &body = fold->getBody();
          auto &accum_arg = fold->getAccumulatorArgument();

          // If the accumulator is encoded, then the result of the fold is
          // encoded.
          if (encoded[&body].count(&accum_arg)) {
            auto *result = &fold->getResult();
            encoded[fold->getFunction()].insert(result);
            worklist.push(result);
            ++count;
          }

          // Otherwise, recurse on the accumulator.
          else {
            worklist.push(&accum_arg);
          }
        }

      } else if (auto *fold = into<FoldInst>(user)) {
        auto &body = fold->getBody();
        auto &body_encoded = encoded[&body];

        if (&use == &fold->getInitialAsUse()) {
          auto &accum_arg = fold->getAccumulatorArgument();

          if (body_encoded.count(&accum_arg)) {
            continue;
          }

          body_encoded.insert(&accum_arg);
          worklist.push(&accum_arg);

        } else if (auto *closed_arg = fold->getClosedArgument(use)) {
          if (body_encoded.count(closed_arg)) {
            continue;
          }

          body_encoded.insert(closed_arg);
          worklist.push(closed_arg);
          ++count;
        }
      } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
#if 0
        
        // Skip memoir calls.
        if (into<MemOIRInst>(call)) {
          continue;
        }

        // Fetch the callee function, if this is a direct call.
        auto *callee = call->getCalledFunction();
        if (not callee) {
          continue;
        }

        // TODO: If we want to be conservative, we should ensure that this
        // function has internal linkage.

        // Fetch the argument information.
        auto arg_no = use.getOperandNo();
        auto *arg = callee->getArg(arg_no);

        println("ARG ENCODED? ", *arg, " IN ", callee->getName());

        // Merge all caller information.
        bool all_encoded = true;
        for (auto &callee_use : callee->uses()) {
          // Fetch the user.
          auto *callee_user = callee_use.getUser();
          if (auto *ret_phi = into<RetPHIInst>(callee_user)) {
            // Skip RetPHIs.
            continue;
          } else if (auto *caller = dyn_cast<llvm::CallBase>(callee_user)) {
            // Fetch the parent function of the caller.
            auto &caller_func =
                MEMOIR_SANITIZE(caller->getFunction(),
                                "Could not get the caller function!");

            // Get the corresponding argument usage.
            auto *caller_arg = caller->getArgOperand(arg_no);

            // Check if the arg operand is encoded.
            auto &caller_encoded = encoded[&caller_func];
            if (not caller_encoded.count(caller->getArgOperand(arg_no))) {
              println("  NO, caller not encoded in ", caller_func.getName());
              all_encoded = false;
              break;
            }
          } else {
            // If we see a non-direct call use of the callee, then we must be
            // conservative and not propagate information.
            println("  NO, found indirect call");
            all_encoded = false;
            break;
          }
        }

        // If all incoming callers pass an encoded value, then propagate.
        if (all_encoded) {
          println("YES!");

          local_encoded.insert(arg);
          worklist.push(arg);
          ++count;
        }
#endif
      }
    }
  }

  return count;
}

llvm::GlobalVariable &create_global_ptr(llvm::Module &module,
                                        const llvm::Twine &name,
                                        bool is_external) {
  auto *llvm_ptr_type = llvm::PointerType::get(module.getContext(), 0);
  auto linkage = is_external ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
                             : llvm::GlobalValue::LinkageTypes::InternalLinkage;
  auto *init = llvm::Constant::getNullValue(llvm_ptr_type);
  auto *global = new llvm::GlobalVariable(module,
                                          llvm_ptr_type,
                                          /* isConstant? */ false,
                                          linkage,
                                          init,
                                          name);
  return MEMOIR_SANITIZE(global, "Failed to allocate new global variable");
}

llvm::AllocaInst &create_stack_ptr(llvm::Function &function,
                                   const llvm::Twine &name) {

  llvm::IRBuilder<> builder(function.getEntryBlock().getFirstNonPHI());

  auto *stack = builder.CreateAlloca(builder.getPtrTy(0), NULL, name);

  return MEMOIR_SANITIZE(stack, "Failed to create alloca");
}

} // namespace folio
