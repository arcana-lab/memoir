// LLVM

// MemOIR
#include "memoir/analysis/SizeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {
SizeAnalysis::~SizeAnalysis() {
  // Do nothing.
}

// Top-level analysis driver.
ValueExpression *SizeAnalysis::getSize(Collection &C) {
  // Visit the collection.
  return this->visitCollection(C);
}

// Analysis visitors.
ValueExpression *SizeAnalysis::visitBaseCollection(BaseCollection &C) {
  // Ensure that we are looking at a sequence collection.
  auto &alloc_inst = C.getAllocation();
  auto *sequence_alloc_inst = dyn_cast<SequenceAllocInst>(&alloc_inst);
  if (sequence_alloc_inst == nullptr) {
    return nullptr;
  }

  // Get the size operand.
  auto &size_operand = sequence_alloc_inst->getSizeOperand();

  // Get the ValueExpression for it.
  auto *size_expr = this->VN.get(size_operand);

  return size_expr;
}

ValueExpression *SizeAnalysis::visitFieldArray(FieldArray &C) {
  return nullptr;
}

ValueExpression *SizeAnalysis::visitNestedCollection(NestedCollection &C) {
  return nullptr;
}

ValueExpression *SizeAnalysis::visitReferencedCollection(
    ReferencedCollection &C) {
  return nullptr;
}

ValueExpression *SizeAnalysis::visitControlPHICollection(
    ControlPHICollection &C) {
  // Visit each of the incoming edges.
  vector<ValueExpression *> incoming_expressions = {};
  vector<llvm::BasicBlock *> incoming_blocks = {};
  for (auto incoming_idx = 0; incoming_idx < C.getNumIncoming();
       incoming_idx++) {
    // Get the incoming basic block.
    auto &incoming_basic_block = C.getIncomingBlock(incoming_idx);

    // Get the incoming collection.
    auto &incoming_collection = C.getIncomingCollection(incoming_idx);

    // Get the size expression.
    auto *incoming_expr = this->visitCollection(incoming_collection);

    // Append it to the Control PHI.
    incoming_expressions.push_back(incoming_expr);
    incoming_blocks.push_back(&incoming_basic_block);
  }

  // If there is only one incoming value, just return that expression.
  if (incoming_expressions.size() == 1) {
    return incoming_expressions.at(0);
  }

  // Create the PHIExpression.
  auto *phi_expr = new PHIExpression(incoming_expressions, incoming_blocks);
  MEMOIR_NULL_CHECK(phi_expr, "Could not construct the PHI Expression");

  return phi_expr;
}

ValueExpression *SizeAnalysis::visitRetPHICollection(RetPHICollection &C) {
  // Visit each of the incoming edges.
  vector<ValueExpression *> incoming_expressions = {};
  vector<llvm::ReturnInst *> incoming_returns = {};
  for (auto incoming_idx = 0; incoming_idx < C.getNumIncoming();
       incoming_idx++) {
    // Get the incoming basic block.
    auto &incoming_return = C.getIncomingReturn(incoming_idx);

    // Get the incoming collection.
    auto &incoming_collection =
        C.getIncomingCollectionForReturn(incoming_return);

    // Get the size expression.
    auto *incoming_expr = this->visitCollection(incoming_collection);

    // Append it to the Control PHI.
    incoming_expressions.push_back(incoming_expr);
    incoming_returns.push_back(&incoming_return);
  }

  // If there is only one incoming value, just return that expression.
  if (incoming_expressions.size() == 1) {
    return incoming_expressions.at(0);
  }

  // Otherwise, we don't have a way to construct interprocedural expressions
  // like this.
  // NOTE: We are normalizing to a single return with noelle-norm so this case
  // shouldn't show up.
  return nullptr;
}

ValueExpression *SizeAnalysis::visitArgPHICollection(ArgPHICollection &C) {
  // TODO: We need to have a way to represent interprocedural ValueExpressions
  // for ArgPHI and RetPHI.
  // NOTE: For now, we will only handle the case where there is one incoming
  // call for the argument.
  if (C.getNumIncoming() != 1) {
    return nullptr;
  }

  // Get the incoming collection.
  auto &incoming_collection = C.getIncomingCollection(0);

  // Get its size expression.
  auto *incoming_expr = this->visitCollection(incoming_collection);

  // Return it.
  return incoming_expr;
}

ValueExpression *SizeAnalysis::visitDefPHICollection(DefPHICollection &C) {
  // Visit the collection being redefined.
  return this->visitCollection(C.getCollection());
}

ValueExpression *SizeAnalysis::visitUsePHICollection(UsePHICollection &C) {
  // Visit the collection being used.
  return this->visitCollection(C.getCollection());
}

ValueExpression *SizeAnalysis::visitJoinPHICollection(JoinPHICollection &C) {
  // Get the size expressions for each of the incoming collections.
  vector<ValueExpression *> joined_expressions = {};
  for (auto join_idx = 0; join_idx < C.getNumberOfJoinedCollections();
       join_idx++) {
    // Get the joined collection.
    auto &joined_collection = C.getJoinedCollection(join_idx);

    // Get the size expression.
    auto *joined_size_expr = this->visitCollection(joined_collection);

    // Append it to the list of joined size expressions.
    joined_expressions.push_back(joined_size_expr);
  }

  // Create the new size expression.
  // NOTE: This is an add of all joined size expressions.
  ValueExpression *join_size_expr = nullptr;
  for (auto joined_expr : joined_expressions) {
    // If this is the first iteration, set the size expression to be the first
    // joined expression.
    if (join_size_expr == nullptr) {
      join_size_expr = joined_expr;
    }

    // Otherwise, create an add of the recurrence and the current joined
    // expression.
    auto *add_expr = new BasicExpression(llvm::Instruction::Add);
    add_expr->arguments.push_back(join_size_expr);
    add_expr->arguments.push_back(joined_expr);

    // Update the recurrent value.
    join_size_expr = add_expr;
  }

  // Return the join size expression.
  return join_size_expr;
}

ValueExpression *SizeAnalysis::visitSliceCollection(SliceCollection &C) {
  // Get the begin and end slice operands.
  auto &slice_inst = C.getSlice();
  auto &begin_operand = slice_inst.getBeginIndex();
  auto &end_operand = slice_inst.getEndIndex();

  // Get the integer type for the indices.
  auto *llvm_type = begin_operand.getType();
  auto *integer_type = dyn_cast<llvm::IntegerType>(llvm_type);
  MEMOIR_NULL_CHECK(integer_type,
                    "Index type for slice collection are not integers!");

  // Check if either of the operands are constant integers.
  auto *begin_const_int = dyn_cast<llvm::ConstantInt>(&begin_operand);
  auto *end_const_int = dyn_cast<llvm::ConstantInt>(&end_operand);

  // If both of the operands are constants and non-negative, then compute the
  // difference and return a ConstantExpression.
  if (begin_const_int != nullptr && end_const_int != nullptr) {
    auto begin_value = begin_const_int->getSExtValue();
    auto end_value = end_const_int->getSExtValue();

    // If both the begin and end are positive constants, compute the size.
    if (begin_value >= 0 && end_value >= 0) {
      int64_t diff_value;
      if (end_value > begin_value) {
        diff_value = end_value - begin_value;
      } else {
        diff_value = begin_value - end_value;
      }

      // Create the constant integer for the difference value.
      auto *diff_const_int =
          ConstantInt::get(integer_type, diff_value, true /* is signed? */);
      MEMOIR_NULL_CHECK(
          diff_const_int,
          "Couldn't construct the constant integer for the difference");

      // Get the ValueExpression for the constant and return.
      auto *diff_size_expr = this->VN.get(*diff_const_int);
      MEMOIR_NULL_CHECK(diff_size_expr, "Couldn't number the constant value");

      // Return the size expression.
      return diff_size_expr;
    }
  }

  // Check if either of the operands are -1, indicating the end of the
  // collection.
  ValueExpression *begin_expr = nullptr;
  if (begin_const_int != nullptr) {
    auto begin_value = begin_const_int->getSExtValue();
    if (begin_value < 0) {
      auto *size_expr = this->visitCollection(C.getSlicedCollection());
      MEMOIR_NULL_CHECK(
          size_expr,
          "Could not determine the size expression for the sliced collection");
      if (begin_value == -1) {
        begin_expr = size_expr;
      } else {
        // Get the offset as a constant.
        auto offset_value = (begin_value + 1) * -1;
        auto &offset_const_int = sanitize(
            ConstantInt::get(integer_type, offset_value, true /* is signed? */),
            "Could not get the offset constant integer");
        auto *offset_expr = this->VN.get(offset_const_int);
        MEMOIR_NULL_CHECK(offset_expr,
                          "Could not get the ValueExpression for the constant");

        // Create a subtract expression for the offset into the size.
        auto &size_minus_offset_expr =
            sanitize(new BasicExpression(llvm::Instruction::Sub),
                     "Could not create the subtract expression");
        size_minus_offset_expr.arguments.push_back(size_expr);
        size_minus_offset_expr.arguments.push_back(offset_expr);

        // Set the begin expression to be the subtraction.
        begin_expr = &size_minus_offset_expr;
      }
    }
  }
  if (begin_expr == nullptr) {
    begin_expr = this->VN.get(begin_operand);
  }

  ValueExpression *end_expr = nullptr;
  if (end_const_int != nullptr) {
    auto end_value = end_const_int->getSExtValue();
    if (end_value < 0) {
      auto *size_expr = this->visitCollection(C.getSlicedCollection());
      MEMOIR_NULL_CHECK(
          size_expr,
          "Could not determine the size expression for the sliced collection");
      if (end_value == -1) {
        end_expr = size_expr;
      } else {
        // Get the offset as a constant.
        auto offset_value = (end_value + 1) * -1;
        auto &offset_const_int = sanitize(
            ConstantInt::get(integer_type, offset_value, true /* is signed? */),
            "Could not get the offset constant integer");
        auto *offset_expr = this->VN.get(offset_const_int);
        MEMOIR_NULL_CHECK(offset_expr,
                          "Could not get the ValueExpression for the constant");

        // Create a subtract expression for the offset into the size.
        auto &size_minus_offset_expr =
            sanitize(new BasicExpression(llvm::Instruction::Sub),
                     "Could not create the subtract expression");
        size_minus_offset_expr.arguments.push_back(size_expr);
        size_minus_offset_expr.arguments.push_back(offset_expr);

        // Set the end expression to be the subtraction.
        end_expr = &size_minus_offset_expr;
      }
    }
  }
  if (end_expr == nullptr) {
    end_expr = this->VN.get(end_operand);
  }

  // Create the subtract expression.
  // TODO: this doesn't work for reverse slices, need to add an absolute value
  // select expression.
  // TODO: extend this to give you the loop-summarized size of the given value.
  auto &size_difference_expr = sanitize(
      new BasicExpression(llvm::Instruction::Sub),
      "Could not create the subtract expression for begin and end difference");
  size_difference_expr.arguments.push_back(begin_expr);
  size_difference_expr.arguments.push_back(end_expr);

  return &size_difference_expr;
}

} // namespace llvm::memoir
