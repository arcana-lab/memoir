#include "memoir/utility/FunctionNames.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "SSADestruction.hpp"

namespace llvm::memoir {

SSADestructionVisitor::SSADestructionVisitor(llvm::noelle::DomTreeSummary &DT,
                                             LivenessAnalysis &LA,
                                             ValueNumbering &VN,
                                             SSADestructionStats *stats)
  : DT(DT),
    LA(LA),
    VN(VN),
    stats(stats) {
  // Do nothing.
}

void SSADestructionVisitor::visitInstruction(llvm::Instruction &I) {
  return;
}

void SSADestructionVisitor::visitUsePHIInst(UsePHIInst &I) {
  auto &used_collection = I.getUsedCollectionOperand();
  auto &collection = I.getCollectionValue();

  this->coalesce(collection, used_collection);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitDefPHIInst(DefPHIInst &I) {
  auto &defined_collection = I.getDefinedCollectionOperand();
  auto &collection = I.getCollectionValue();

  this->coalesce(collection, defined_collection);

  this->markForCleanup(I);

  return;
}

static void slice_to_view(SliceInst &I) {
  auto &call_inst = I.getCallInst();
  auto *view_func = FunctionNames::get_memoir_function(*call_inst.getModule(),
                                                       MemOIR_Func::VIEW);
  MEMOIR_NULL_CHECK(view_func, "Could not find the memoir view function");
  call_inst.setCalledFunction(view_func);
  return;
}

void SSADestructionVisitor::visitSliceInst(SliceInst &I) {
  auto &collection = I.getCollectionOperand();
  auto &slice = I.getSliceAsValue();

  // If the collection is dead immediately following,
  // then we can replace this slice with a view.
  if (!this->LA.is_live(collection, I)) {
    slice_to_view(I);
    return;
  }

  // If the slice is disjoint from the live slice range of the collection.
  bool is_disjoint = true;
  auto &slice_begin = I.getBeginIndex();
  auto &slice_end = I.getEndIndex();
  set<SliceInst *> slice_users = {};
  for (auto *user : collection.users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      // This is an overly conservative check.
      is_disjoint = false;
      break;
    }

    // Check that the user is dominated by this instruction.
    if (!DT.dominates(&I.getCallInst(), user_as_inst)) {
      break;
    }

    auto *user_as_memoir = MemOIRInst::get(*user_as_inst);
    if (!user_as_memoir) {
      // Also overly conservative, we _can_ handle PHIs.
      is_disjoint = false;
      break;
    }

    if (auto *user_as_slice = dyn_cast<SliceInst>(user_as_memoir)) {
      // We will check all slice users for non-overlapping index spaces, _if_
      // there are no other users.
      slice_users.insert(user_as_slice);
      continue;
    }

    if (auto *user_as_access = dyn_cast<AccessInst>(user_as_memoir)) {
      // TODO: check the interval range of the index.
    }

    is_disjoint = false;
    break;
  }

  if (!is_disjoint) {
    return;
  }

  slice_users.erase(&I);

  // Check the slice users to see if they are non-overlapping.
  if (!slice_users.empty()) {
    auto &slice_begin = I.getBeginIndex();
    auto &slice_end = I.getEndIndex();
    set<SliceInst *> visited = {};
    list<llvm::Value *> limits = { &slice_begin, &slice_end };
    debugln("check non-overlapping");
    while (visited.size() < slice_users.size()) {
      bool found_new_limit = false;
      for (auto *user_as_slice : slice_users) {
        if (visited.find(user_as_slice) != visited.end()) {
          continue;
        }

        auto &user_slice_begin = user_as_slice->getBeginIndex();
        auto &user_slice_end = user_as_slice->getEndIndex();

        debugln("lower limit: ", *limits.front());
        debugln("upper limit: ", *limits.back());
        debugln(" user begin: ", user_slice_begin);
        debugln(" user   end: ", user_slice_end);

        // Check if this slice range is overlapping.
        if (&user_slice_begin == limits.back()) {
          visited.insert(user_as_slice);
          limits.push_back(&user_slice_end);
          found_new_limit = true;
        } else if (&user_slice_end == limits.front()) {
          visited.insert(user_as_slice);
          limits.push_front(&user_slice_begin);
          found_new_limit = true;
        }
      }

      // If we found a new limit, continue working.
      if (!found_new_limit) {
        break;
      }
    }

    // Otherwise, we need to bring out the big guns and check for relations.
    auto *slice_begin_expr = this->VN.get(slice_begin);
    auto *slice_end_expr = this->VN.get(slice_end);
    ValueExpression *new_lower_limit = nullptr;
    ValueExpression *new_upper_limit = nullptr;
    for (auto *user_as_slice : slice_users) {
      if (visited.find(user_as_slice) != visited.end()) {
        continue;
      }

      // Check if this slice range is non-overlapping, with an offset.
      auto *user_slice_begin_expr =
          this->VN.get(user_as_slice->getBeginIndex());
      MEMOIR_NULL_CHECK(user_slice_begin_expr,
                        "Error making value expression for begin index");
      auto *user_slice_end_expr = this->VN.get(user_as_slice->getEndIndex());
      MEMOIR_NULL_CHECK(user_slice_end_expr,
                        "Error making value expression for end index");

      debugln("Checking left");
      debugln("  ", user_as_slice->getBeginIndex());
      debugln("  ", slice_begin);
      auto check_left = *user_slice_end_expr < *slice_begin_expr;

      debugln("Checking right");
      debugln("  ", user_as_slice->getEndIndex());
      debugln("  ", slice_end);
      auto check_right = *user_slice_begin_expr > *slice_end_expr;

      if (check_left || check_right) {
        continue;
      } else {
        warnln("Big guns failed,"
               " open an issue if this shouldn't have happened.");
        is_disjoint = false;
      }
    }
  }

  if (is_disjoint) {
    slice_to_view(I);
  }

  return;
}

void SSADestructionVisitor::visitJoinInst(JoinInst &I) {
  auto &collection = I.getCollectionAsValue();
  auto num_joined = I.getNumberOfJoins();

  // For each join operand, if it is dead after the join, we can coallesce this
  // join with it.
  bool all_dead = true;
  for (auto join_idx = 0; join_idx < num_joined; join_idx++) {
    auto &joined_use = I.getJoinedOperandAsUse(join_idx);
    auto &joined_collection =
        MEMOIR_SANITIZE(joined_use.get(), "Use by join is NULL!");
    if (this->LA.is_live(joined_collection, I)) {
      all_dead = false;

      debugln("Live after join!");
      debugln("        ", joined_collection);
      debugln("  after ", I);

      break;
    }
  }

  if (!all_dead) {
    infoln("Not all incoming values are dead after a join.");
    infoln(" |-> ", I);
    return;
  }

  // If all join operands are dead, convert the join to a series of appends.
  // if (all_dead) {
  //   auto &first_collection = I.getJoinedOperand(0);
  //   MemOIRBuilder builder(I);
  //   for (auto join_idx = 1; join_idx < num_joined; join_idx++) {
  //     auto &joined_collection = I.getJoinedOperand(join_idx);
  //     debugln("Creating append");
  //     debugln("  ", first_collection);
  //     debugln("  ", joined_collection);

  //     builder.CreateSeqAppendInst(&first_collection, &joined_collection);
  //   }

  //   // Coalesce the first operand and the join.
  //   this->coalesce(I, first_collection);

  //   // The join is dead after coalescence.
  //   this->markForCleanup(I);

  //   return;
  // }

  // If all operands of the join are views of the same collection:
  //  - If views are in order, coallesce resultant and the viewed collection.
  //  - Otherwise, determine if the size is the same after the join:
  //     - If the size is the same, convert to a swap.
  //     - Otherwise, convert to a remove.
  vector<ViewInst *> views = {};
  views.reserve(num_joined);
  bool all_views = true;
  llvm::Value *base_collection = nullptr;
  for (auto join_idx = 0; join_idx < num_joined; join_idx++) {
    auto &joined_use = I.getJoinedOperandAsUse(join_idx);
    auto &joined_collection =
        MEMOIR_SANITIZE(joined_use.get(), "Use by join is NULL!");

    auto *joined_as_inst = dyn_cast<llvm::Instruction>(&joined_collection);
    if (!joined_as_inst) {
      base_collection = nullptr;
      break;
    }

    auto *joined_collection_as_memoir = MemOIRInst::get(*joined_as_inst);
    llvm::Value *used_collection;
    if (auto *view_inst =
            dyn_cast_or_null<ViewInst>(joined_collection_as_memoir)) {
      auto &viewed_collection = view_inst->getCollectionOperand();
      used_collection = &viewed_collection;
      views.push_back(view_inst);
    } else {
      infoln("Join uses non-view");
      infoln(" |-> ", I);
      all_views = false;
      continue;
    }

    // See if we are still using the same base collection.
    if (base_collection == nullptr) {
      base_collection = used_collection;
      continue;
    } else if (base_collection == used_collection) {
      continue;
    } else {
      base_collection = nullptr;
      break;
    }
  }

  if (base_collection != nullptr && all_views) {
    println("Join uses a single base collection");
    println(" |-> ", I);

    // Determine if the size is the same.
    set<ViewInst *> visited = {};
    list<size_t> view_order = {};
    list<pair<llvm::Value *, llvm::Value *>> ranges_to_remove = {};
    llvm::Value *lower_limit = nullptr;
    llvm::Value *upper_limit = nullptr;
    bool swappable = true;
    while (visited.size() != views.size()) {
      bool new_limit = false;
      for (auto view_idx = 0; view_idx < views.size(); view_idx++) {
        auto *view = views[view_idx];
        auto &begin_index = view->getBeginIndex();
        auto &end_index = view->getEndIndex();
        if (lower_limit == nullptr) {
          lower_limit = &begin_index;
          upper_limit = &end_index;
          visited.insert(view);
          view_order.push_back(view_idx);
          new_limit = true;
          continue;
        } else if (lower_limit == &end_index) {
          lower_limit = &begin_index;
          visited.insert(view);
          view_order.push_front(view_idx);
          new_limit = true;
          continue;
        } else if (upper_limit == &begin_index) {
          upper_limit = &end_index;
          visited.insert(view);
          view_order.push_back(view_idx);
          new_limit = true;
          continue;
        }
      }

      if (new_limit) {
        continue;
      }

      // Determine the ranges that must be removed.
      auto *lower_limit_expr = this->VN.get(*lower_limit);
      auto *upper_limit_expr = this->VN.get(*upper_limit);
      ValueExpression *new_lower_limit_expr = nullptr;
      ViewInst *lower_limit_view = nullptr;
      ValueExpression *new_upper_limit_expr = nullptr;
      ViewInst *upper_limit_view = nullptr;
      for (auto view_idx = 0; view_idx < views.size(); view_idx++) {
        auto *view = views[view_idx];
        if (visited.find(view) != visited.end()) {
          continue;
        }

        // Find the nearest
        auto &view_begin_expr =
            MEMOIR_SANITIZE(this->VN.get(view->getBeginIndex()),
                            "Error making value expression for begin index");
        auto &view_end_expr =
            MEMOIR_SANITIZE(this->VN.get(view->getEndIndex()),
                            "Error making value expression for end index");

        auto check_left = view_end_expr < *lower_limit_expr;
        auto check_right = view_begin_expr > *upper_limit_expr;

        if (check_left) {
          // Check if this is the new lower bound.
          if (new_lower_limit_expr == nullptr) {
            new_lower_limit_expr = &view_end_expr;
            lower_limit_view = view;
            new_limit = true;
          } else if (view_end_expr < *new_lower_limit_expr) {
            new_lower_limit_expr = &view_end_expr;
            lower_limit_view = view;
            new_limit = true;
          }
          continue;
        } else if (check_right) {
          // Check if this is the new upper bound.
          if (new_upper_limit_expr == nullptr) {
            new_upper_limit_expr = &view_begin_expr;
            upper_limit_view = view;
            new_limit = true;
          } else if (view_begin_expr > *new_upper_limit_expr) {
            new_upper_limit_expr = &view_begin_expr;
            upper_limit_view = view;
            new_limit = true;
          }
          continue;
        }
      }

      // If we found a new limit, update the lower and/or upper limit and mark
      // the range for removal.
      if (new_limit) {
        if (lower_limit_view != nullptr) {
          auto &new_lower_limit = lower_limit_view->getEndIndex();
          ranges_to_remove.push_front(make_pair(&new_lower_limit, lower_limit));
          lower_limit = &new_lower_limit;
        }

        if (upper_limit_view != nullptr) {
          auto &new_upper_limit = upper_limit_view->getBeginIndex();
          ranges_to_remove.push_back(make_pair(upper_limit, &new_upper_limit));
          upper_limit = &new_upper_limit;
        }

        // Keep on chugging.
        continue;
      }

      println("!!!!");
      println("The join: ", I);
      MEMOIR_UNREACHABLE(
          "We shouldn't be here, couldnt' find out what to swap/remove forthe above join!");
    }

    if (swappable) {
      infoln("Convert the join to a swap.");
      infoln(" |-> ", I);
      infoln(" |-> Original order:");
      for (auto view_idx : view_order) {
        infoln(" | |-> ", *views[view_idx]);
      }
      infoln(" |-> New order:");
      for (auto view : views) {
        infoln(" | |-> ", *view);
      }

      auto new_idx = 0;
      auto i = 0;
      for (auto it = view_order.begin(); it != view_order.end(); ++it, i++) {
        auto orig_idx = *it;
        if (orig_idx != new_idx) {
          println(" |-> Swap ", orig_idx, " <=> ", new_idx);

          // Check that these views are the same size.
          auto &from_begin = views[orig_idx]->getBeginIndex();
          auto *from_begin_expr = this->VN.get(from_begin);
          auto &from_end = views[orig_idx]->getEndIndex();
          auto *from_end_expr = this->VN.get(from_end);
          auto &to_begin = views[new_idx]->getBeginIndex();
          auto *to_begin_expr = this->VN.get(to_begin);
          auto &to_end = views[new_idx]->getEndIndex();
          auto *to_end_expr = this->VN.get(to_end);

          // Create the expression for the size calculation.
          auto *from_size_expr = new BasicExpression(
              llvm::Instruction::Sub,
              vector<ValueExpression *>({ from_end_expr, from_begin_expr }));
          auto *to_size_expr = new BasicExpression(
              llvm::Instruction::Sub,
              vector<ValueExpression *>({ to_end_expr, to_begin_expr }));

          // If they are the same size, swap them.
          if (*from_size_expr == *to_size_expr) {
            // Create the swap.
            MemOIRBuilder builder(I);
            builder.CreateSeqSwapInst(base_collection,
                                      &from_begin,
                                      &from_end,
                                      &to_begin);

            // Swap the operands.
            auto &from_use = I.getJoinedOperandAsUse(orig_idx);
            auto &to_use = I.getJoinedOperandAsUse(new_idx);
            from_use.swap(to_use);

            // NOTE: we don't need to update other users here, as these views
            // were already checked to be dead following.

            // Update the view order post-swap.
            auto r = view_order.size() - 1;
            for (auto rit = view_order.rbegin(); r > i; ++rit, r--) {
              if (*rit == new_idx) {
                std::swap(*(views.begin() + i), *(views.begin() + r));
                std::swap(*it, *rit);
                break;
              }
            }
          } else {
            MEMOIR_UNREACHABLE("Size of from and to for swap are not the same! "
                               "Someone must have changed the program.");
          }
        }
        new_idx++;
      }

      // Replace the join with the base collection that has been swapped.
      this->coalesce(I, *base_collection);
      this->markForCleanup(I);
    }

    // Now to remove any ranges that were marked.
    for (auto range : ranges_to_remove) {
      auto *begin = range.first;
      auto *end = range.second;
      MemOIRBuilder builder(I);
      // TODO: check if this range is alive as a view, if it is we need to
      // convert it to a slice.
      builder.CreateSeqRemoveInst(base_collection, begin, end);
    }
  }

  // Otherwise, some operands are views from a different collection:
  //  - If the size is the same, convert to a swap.
  //  - Otherwise, convert to an append.

  return;
}

void SSADestructionVisitor::cleanup() {
  for (auto *inst : instructions_to_delete) {
    infoln(*inst);
    inst->eraseFromParent();
  }
}

void SSADestructionVisitor::coalesce(MemOIRInst &I, llvm::Value &replacement) {
  this->coalesce(I.getCallInst(), replacement);
}

void SSADestructionVisitor::coalesce(llvm::Value &V, llvm::Value &replacement) {
  infoln("Coalesce:");
  infoln("  ", V);
  infoln("  ", replacement);
  this->coalesced_values[&V] = &replacement;
}

llvm::Value *SSADestructionVisitor::find_replacement(llvm::Value *value) {
  auto *replacement_value = value;
  auto found = this->replaced_values.find(value);
  while (found != this->replaced_values.end()) {
    replacement_value = found->second;
    found = this->replaced_values.find(replacement_value);
  }
  return replacement_value;
}

void SSADestructionVisitor::do_coalesce(llvm::Value &V) {
  auto found_coalesce = this->coalesced_values.find(&V);
  if (found_coalesce == this->coalesced_values.end()) {
    return;
  }

  auto *replacement = this->find_replacement(found_coalesce->second);

  infoln("Coalescing:");
  infoln("  ", V);
  infoln("  ", *replacement);

  V.replaceAllUsesWith(replacement);

  this->replaced_values[&V] = replacement;
}

void SSADestructionVisitor::markForCleanup(MemOIRInst &I) {
  this->markForCleanup(I.getCallInst());
}

void SSADestructionVisitor::markForCleanup(llvm::Instruction &I) {
  this->instructions_to_delete.insert(&I);
}

} // namespace llvm::memoir
