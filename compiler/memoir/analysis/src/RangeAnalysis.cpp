#include "memoir/analysis/RangeAnalysis.hpp"

#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

RangeAnalysis::RangeAnalysis(llvm::Function &F, arcana::noelle::Noelle &noelle)
  : F(F),
    noelle(noelle) {
  // Analyze the function.
  auto success = this->analyze(this->F, this->noelle);
  MEMOIR_ASSERT((success == true), "Range analysis failed.");
}

void RangeAnalysis::propagate_range_to_uses(ValueRange &range,
                                            const set<llvm::Use *> &uses) {
  for (auto *use : uses) {
    this->use_to_range[use] = &range;
  }
}

static ValueExpression &llvm_value_to_expr(llvm::Value &V) {
  if (auto *constant = dyn_cast<llvm::Constant>(&V)) {
    return *(new ConstantExpression(*constant));
  } else {
    return *(new VariableExpression(V));
  }
}

ValueRange &RangeAnalysis::induction_variable_to_range(
    arcana::noelle::LoopGoverningInductionVariable &LGIV) {
  // Get the induction variable.
  auto &IV = MEMOIR_SANITIZE(
      LGIV.getInductionVariable(),
      "Loop-Governing Induction Variable has NULL Induction Variable!");

  // Get the start value for the induction variable.
  if (auto *start_value = IV.getStartValue()) {
    // If the step value is positive, then get the exit condition value.
    if (IV.isStepValuePositive()) {
      if (auto *exit_value = LGIV.getExitConditionValue()) {
        // Create and return the range [start_value, exit_condition_value).
        // TODO: may need to inspect the compare instruction to handle all edge
        // cases.
        auto *lower_expr = &llvm_value_to_expr(*start_value);
        auto *upper_expr = &llvm_value_to_expr(*exit_value);
        return this->create_value_range(*lower_expr, *upper_expr);
      }
    }
  }

  return this->create_overdefined_range();
}

ValueRange &RangeAnalysis::induction_variable_to_range(
    arcana::noelle::InductionVariable &IV,
    arcana::noelle::LoopGoverningInductionVariable &LGIV) {
  // If the induction variable _is_ the loop-governing induction variable, go to
  // special handling for it.
  if (&IV == LGIV.getInductionVariable()) {
    return this->induction_variable_to_range(LGIV);
  }

  // TODO: implement me.
  // TODO: just create a max to min of me.
  return this->create_overdefined_range();
}

static void add_index_use(
    map<llvm::Value *, set<llvm::Use *>> &index_value_to_uses,
    llvm::Use &index_use) {
  index_value_to_uses[index_use.get()].insert(&index_use);
}

bool RangeAnalysis::analyze(llvm::Function &F, arcana::noelle::Noelle &noelle) {
  // Collect all index values used by memoir instructions.
  map<llvm::Value *, set<llvm::Use *>> index_value_to_uses = {};
  for (auto &BB : F) {
    for (auto &I : BB) {
      auto *memoir_inst = MemOIRInst::get(I);
      if (!memoir_inst) {
        continue;
      }

      if (auto *read_inst = dyn_cast<IndexReadInst>(memoir_inst)) {
        for (auto dim_idx = 0; dim_idx < read_inst->getNumberOfDimensions();
             ++dim_idx) {
          auto &index_use = read_inst->getIndexOfDimensionAsUse(dim_idx);
          add_index_use(index_value_to_uses, index_use);
        }
      } else if (auto *write_inst = dyn_cast<IndexWriteInst>(memoir_inst)) {
        for (auto dim_idx = 0; dim_idx < write_inst->getNumberOfDimensions();
             ++dim_idx) {
          auto &index_use = write_inst->getIndexOfDimensionAsUse(dim_idx);
          add_index_use(index_value_to_uses, index_use);
        }
      } else if (auto *get_inst = dyn_cast<IndexGetInst>(memoir_inst)) {
        for (auto dim_idx = 0; dim_idx < get_inst->getNumberOfDimensions();
             ++dim_idx) {
          auto &index_use = get_inst->getIndexOfDimensionAsUse(dim_idx);
          add_index_use(index_value_to_uses, index_use);
        }
      } else if (auto *insert_inst = dyn_cast<SeqInsertInst>(memoir_inst)) {
        add_index_use(index_value_to_uses,
                      insert_inst->getInsertionPointAsUse());

      } else if (auto *insert_seq_inst =
                     dyn_cast<SeqInsertSeqInst>(memoir_inst)) {
        auto &index_value_use = insert_seq_inst->getInsertionPointAsUse();
        add_index_use(index_value_to_uses, index_value_use);
      } else if (auto *remove_inst = dyn_cast<SeqRemoveInst>(memoir_inst)) {
        add_index_use(index_value_to_uses, remove_inst->getBeginIndexAsUse());
        add_index_use(index_value_to_uses, remove_inst->getEndIndexAsUse());
        // } else if (auto *swap_inst = dyn_cast<SwapInst>(memoir_inst)) {
        //   index_value_to_uses[&I].insert({ &swap_inst->getBeginIndexAsUse(),
        //                                    &swap_inst->getEndIndexAsUse(),
        //                                    &swap_inst->getToBeginIndexAsUse()
        //                                    });
      } else if (auto *copy_inst = dyn_cast<SeqCopyInst>(memoir_inst)) {
        add_index_use(index_value_to_uses, copy_inst->getBeginIndexAsUse());
        add_index_use(index_value_to_uses, copy_inst->getEndIndexAsUse());
      }
    }
  }

  // Get all the loops in the function.
  auto &loops = MEMOIR_SANITIZE(
      noelle.getLoopContents(&F),
      "NOELLE gave us NULL instead of a vector of loop structures!");

  // For each index value, determine the range of its uses:
  for (const auto &[index_value, index_uses] : index_value_to_uses) {

    // If the value is a constant integer:
    if (auto *index_const = dyn_cast<llvm::ConstantInt>(index_value)) {
      auto &const_expr =
          MEMOIR_SANITIZE(new ConstantExpression(*index_const),
                          "Failed to create ConstantExpression!");
      this->propagate_range_to_uses(
          this->create_value_range(const_expr, const_expr),
          index_uses);
      continue;
    }

    // Otherwise, if the value is an argument:
    else if (auto *index_arg = dyn_cast<llvm::Argument>(index_value)) {
      auto &arg_expr = MEMOIR_SANITIZE(new ArgumentExpression(*index_arg),
                                       "Failed to create ArgumentExpression!");
      this->propagate_range_to_uses(
          this->create_value_range(arg_expr, arg_expr),
          index_uses);
      continue;
    }

    // Otherwise, if the value is an instruction:
    else if (auto *index_inst = dyn_cast<llvm::Instruction>(index_value)) {
      // Create a mapping from loop structures to induction variables that the
      // index instruction is involved in.
      map<arcana::noelle::LoopStructure *, ValueRange *> loop_to_range = {};
      for (auto *loop : loops) {
        // Get the loop structure.
        auto &loop_structure =
            MEMOIR_SANITIZE(loop->getLoopStructure(),
                            "NOELLE gave us a NULL LoopStructure!");

        // Get the induction variable manager.
        auto &IVM =
            MEMOIR_SANITIZE(loop->getInductionVariableManager(),
                            "NOELLE gave us a NULL InductionVariableManager");

        // Get the loop-governing induction variable, if it exists.
        auto *loop_governing_induction_variable =
            IVM.getLoopGoverningInductionVariable(loop_structure);

        // If the loop has no loop-governing induction variable, skip it.
        if (loop_governing_induction_variable == nullptr) {
          continue;
        }

        // See if the index value is involved in any induction variables of the
        // loop.
        if (auto *induction_variable =
                IVM.getInductionVariable(loop_structure, index_inst)) {
          auto &range = this->induction_variable_to_range(
              *induction_variable,
              *loop_governing_induction_variable);
          loop_to_range[&loop_structure] = &range;
        }
      }

      // For each use, propagate the value's range:
      for (auto *index_use : index_uses) {
        auto *user = index_use->getUser();
        auto *user_as_inst = dyn_cast_or_null<llvm::Instruction>(user);

        // If the use occurs in a loop, determine if the index is an induction
        // variable:
        for (auto const &[loop, range] : loop_to_range) {
          // If the loop contains the user:
          if (loop->isIncluded(user_as_inst)) {
            this->use_to_range[index_use] = range;
          }
        }
      }
    }
  }

  return true;
}

ValueRange &RangeAnalysis::get_value_range(llvm::Use &use) {
  auto found_range = this->use_to_range.find(&use);
  if (found_range != this->use_to_range.end()) {
    auto the_range = found_range->second;
    MEMOIR_ASSERT((the_range != nullptr), "ValueRange is NULL!");
    return *the_range;
  }
  MEMOIR_UNREACHABLE("LLVM Use provided was not analyzed by the Range Analysis"
                     " for the function!");
}

ValueRange &RangeAnalysis::create_value_range(ValueExpression &lower,
                                              ValueExpression &upper) {
  auto &range = MEMOIR_SANITIZE(new ValueRange(lower, upper),
                                "Failed to allocate ValueRange!");
  this->ranges.insert(&range);

  return range;
}

ValueExpression &RangeAnalysis::create_min_expr() {
  auto &context = this->F.getContext();
  // TODO: make this get the size_t from a MemOIRContext.
  auto &size_type = MEMOIR_SANITIZE(llvm::IntegerType::get(context, 64),
                                    "Failed to get LLVM 64-bit integer type!");
  auto &zero_value = MEMOIR_SANITIZE(llvm::ConstantInt::get(&size_type, 0),
                                     "Failed to get LLVM constant zero!");
  auto &zero_expr = MEMOIR_SANITIZE(new ConstantExpression(zero_value),
                                    "Failed to create ConstantExpression!");

  return zero_expr;
}

ValueExpression &RangeAnalysis::create_max_expr() {
  auto &end_expr =
      MEMOIR_SANITIZE(new EndExpression(), "Failed to create EndExpression!");

  return end_expr;
}

ValueRange &RangeAnalysis::create_overdefined_range() {
  return this->create_value_range(this->create_min_expr(),
                                  this->create_max_expr());
}

void RangeAnalysis::dump() {
  println("RangeAnalysis results:");

  for (auto const *range : this->ranges) {
    println("  Range: ", *range);
  }

  for (auto const &[use, range] : this->use_to_range) {
    println("  For use of ", *use->get(), " at ", *use->getUser());
    println("    Range = ", *range);
  }
}

RangeAnalysis::~RangeAnalysis() {
  for (auto *range : this->ranges) {
    delete range;
  }
}

} // namespace llvm::memoir
