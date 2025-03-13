#include <tuple>

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/TypeCheck.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/PassUtils.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/Metadata.hpp"

#include "folio/analysis/ContentAnalysis.hpp"
#include "folio/analysis/ContentSimplification.hpp"

using namespace llvm::memoir;

namespace folio {

// =========================================================================
// Initialization
namespace detail {

ContentSummary conservative(llvm::Value &V) {

  auto *type = type_of(V);

  if (isa_and_nonnull<SequenceType>(type)
      or isa_and_nonnull<AssocArrayType>(type)) {
    return { &Content::create<KeysContent>(V),
             &Content::create<ElementsContent>(V) };
  } else if (isa_and_nonnull<TupleType>(type)) {
    return { &Content::create<UnderdefinedContent>(),
             &Content::create<StructContent>(V) };
  } else {
    return { &Content::create<UnderdefinedContent>(),
             &Content::create<ScalarContent>(V) };
  }
}

ContentSummary conservative(MemOIRInst &I) {
  return detail::conservative(I.getCallInst());
}

} // namespace detail

void ContentAnalysisDriver::summarize(llvm::Value &value,
                                      ContentSummary summary) {
  this->result[&value] = summary;
}

void ContentAnalysisDriver::summarize(MemOIRInst &I, ContentSummary summary) {
  this->summarize(I.getCallInst(), summary);
}

bool ContentAnalysisDriver::is_in_scope(llvm::Value &V) {
  auto *current_function = this->current->getFunction();

  if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return current_function == inst->getFunction();
  } else if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return current_function == arg->getParent();
  }

  // No other values are scoped.
  return true;
}

std::pair<set<llvm::Value *>, bool> ContentAnalysisDriver::set_recurse() {
  std::pair<set<llvm::Value *>, bool> old = { this->visited, this->recurse };

  // this->visited.clear();
  this->recurse = true;

  return old;
}
void ContentAnalysisDriver::restore_recurse(
    std::pair<set<llvm::Value *>, bool> old) {
  // this->visited = old.first;
  this->recurse = old.second;
}

ContentSummary ContentAnalysisDriver::analyze(llvm::Value &V,
                                              bool force_recurse) {

  // Check if this value is in scope.
  auto in_scope = this->is_in_scope(V);

  // If the value is in scope, and we are not in recursive mode, return the
  // conservative contents.
  if (not force_recurse) {
    if (this->current != &V and in_scope and not this->recurse) {
      return detail::conservative(V);
    }
  }

  // Check if we have already visited this value.
  if (isa<llvm::PHINode>(&V) and visited.count(&V) > 0) {
    // Already visited, if we have a temporary summary, return it.
    auto found_temp = this->temporaries.find(&V);
    if (found_temp == this->temporaries.end()) {
      return detail::conservative(V);
    } else {
      return found_temp->second;
    }
  }

  // Mark the value as visited.
  visited.insert(&V);

  // If we haven't, dispatch to the appropriate handler.
  ContentSummary content;
  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    content = this->visitArgument(*arg);

    this->summarize(*arg, content);

  } else if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    content = this->visit(*inst);

  } else {
    content = detail::conservative(V);
  }

  // Save the result as a temporary.
  this->temporaries[&V] = content;

  return content;
}

ContentSummary ContentAnalysisDriver::analyze(MemOIRInst &I,
                                              bool force_recurse) {
  return this->analyze(I.getCallInst(), force_recurse);
}

ContentSummary ContentAnalysisDriver::visitArgument(llvm::Argument &A) {
  // Check if the parent function is the body of a FoldInst.
  auto &function = MEMOIR_SANITIZE(A.getParent(), "Argument has no parent");
  if (auto *use = function.getSingleUndroppableUse()) {
    auto *user = use->getUser();
    if (auto *fold = into<FoldInst>(user)) {
      // Contextualize the argument for the fold.
      auto &operand_use = fold->getOperandForArgument(A);
      auto &operand = MEMOIR_SANITIZE(operand_use.get(), "Operand is NULL.");

      // If the operand is the collection being folded over, construct the
      // keys/domain for it.
      if (operand_use.getOperandNo()
          == fold->getCollectionAsUse().getOperandNo()) {

        auto [operand_domain, operand_range] = detail::conservative(operand);
        auto &empty = Content::create<EmptyContent>();

        // If this is the second argument, it is the key.
        if (&fold->getIndexArgument() == &A) {
          return { &empty, operand_domain };
        } else if (fold->getElementArgument() == &A) {
          return { &empty, operand_range };
        }
      } else {
        // Otherwise, it is the accumulator or a closed argument.

        // Return the content summary of the operand.
        return this->is_in_scope(operand) ? detail::conservative(operand)
                                          : this->analyze(operand);
      }
    }
  }

  // Handle functions with a single call site.
  llvm::CallBase *single_call = nullptr;
  for (auto &use : function.uses()) {
    auto *user = use.getUser();
    if (into<RetPHIInst>(user)) {
      // Ignore RetPHI uses.
      continue;
    } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      if (single_call) {
        single_call = nullptr;
        break;
      } else {
        single_call = call;
        continue;
      }
    } else {
      single_call = nullptr;
      break;
    }
  }
  if (single_call) {
    // Contextualize the argument for the singular caller.
    auto &operand_use = single_call->getArgOperandUse(A.getArgNo());
    auto &operand = MEMOIR_SANITIZE(operand_use.get(), "Operand is NULL.");

    // Get the content summary of the operand.
    return this->is_in_scope(operand) ? detail::conservative(operand)
                                      : this->analyze(operand);
  }

  return detail::conservative(A);
}

ContentSummary ContentAnalysisDriver::visitInstruction(llvm::Instruction &I) {
  return detail::conservative(I);
}

ContentSummary ContentAnalysisDriver::visitSequenceAllocInst(
    SequenceAllocInst &I) {
  auto &empty = Content::create<EmptyContent>();

  return { &Content::create<RangeContent>(I.getCallInst()), &empty };
}

ContentSummary ContentAnalysisDriver::visitAssocArrayAllocInst(
    AssocArrayAllocInst &I) {
  auto &empty = Content::create<EmptyContent>();

  return { &empty, &empty };
}

ContentSummary ContentAnalysisDriver::visitSeqInsertInst(SeqInsertInst &I) {

  // Analyze the input collection.
  auto [base_domain, base_range] = this->analyze(I.getBaseCollection());

  // Analyze the insertion point.
  auto [_, value_range] = this->analyze(I.getInsertionPoint(), true);

  // Create the domain union.
  auto &domain = Content::create<UnionContent>(*value_range, *base_domain);

  // Analyze the collection type.
  auto *type = type_of(I.getResult());
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(type),
                      "SeqInsertInst on non-collection type");
  auto *struct_type = dyn_cast<TupleType>(&collection_type.getElementType());

  // If the element type is a struct, create a TupleContent with empty fields.
  if (struct_type) {
    vector<Content *> fields(struct_type->getNumFields(),
                             &Content::create<EmptyContent>());

    auto &tuple_content = Content::create<TupleContent>(fields);

    auto &range = Content::create<UnionContent>(tuple_content, *base_range);

    return { &domain, &range };
  }

  // Otherwise, the range is the same as the base.
  return { &domain, base_range };
}

ContentSummary ContentAnalysisDriver::visitIndexWriteInst(IndexWriteInst &I) {

  // Analyze the value being written.
  auto [_value_domain, value_range] = this->analyze(I.getValueWritten(), true);

  // If the value written is a non-constant scalar, we will just give up.
  if (auto *scalar = dyn_cast<ScalarContent>(value_range)) {
    if (not isa<llvm::Constant>(&scalar->value())) {
      return detail::conservative(I.getObjectOperand());
    }
  }

  // Analyze the index being written.
  auto [_index_domain, index_range] = this->analyze(I.getIndex(), true);

  // Fetch the content of the input collection.
  auto &collection = I.getObjectOperand();
  auto [domain, base_range] = this->analyze(collection);

  // Get the type information.
  auto &collection_type = I.getCollectionType();
  auto *element_type = &collection_type.getElementType();

  // Handle subindices.
  auto *element_content = value_range;
  for (unsigned sub_dim = 0; sub_dim < I.getNumberOfSubIndices(); ++sub_dim) {

    // Get the struct type.
    auto &struct_type = MEMOIR_SANITIZE(dyn_cast<TupleType>(element_type),
                                        "Subindex access to non-struct type.");

    // Create a TupleContent and union it with the input.
    vector<Content *> fields(struct_type.getNumFields(),
                             &Content::create<EmptyContent>());

    // Update the relevant field.
    auto field_index = I.getSubIndex(sub_dim);
    fields[field_index] = element_content;

    // Create the TupleContent.
    element_content = &Content::create<TupleContent>(fields);

    auto *element_type = &struct_type.getFieldType(field_index);
  }

  auto &range = Content::create<UnionContent>(*element_content, *base_range);

  return { domain, &range };
}

ContentSummary ContentAnalysisDriver::visitSeqInsertValueInst(
    SeqInsertValueInst &I) {

  // Fetch the base collection info.
  auto &base = I.getBaseCollection();
  auto [base_domain, base_range] = this->analyze(base);

  // Fetch the insertion point info.
  auto [_index_domain, index_range] =
      this->analyze(I.getInsertionPoint(), true);

  // Fetch the inserted value's range.
  auto [_value_domain, value_range] = this->analyze(I.getValueInserted(), true);

  // Construct the new domain.
  auto &domain = Content::create<UnionContent>(*index_range, *base_range);

  // Construct the new range.
  auto &range = Content::create<UnionContent>(*value_range, *base_domain);

  return { &domain, &range };
}

ContentSummary ContentAnalysisDriver::visitAssocWriteInst(AssocWriteInst &I) {

  // Fetch the base collection info.
  auto &base = I.getObjectOperand();
  auto [base_domain, base_range] = this->analyze(base);

  // Fetch the inserted value's range.
  auto [_, value_range] = this->analyze(I.getValueWritten(), true);

  // Construct the new range.
  auto &range = Content::create<UnionContent>(*value_range, *base_range);

  return { base_domain, &range };
}

ContentSummary ContentAnalysisDriver::visitAssocInsertInst(AssocInsertInst &I) {

  // Fetch the base collection info.
  // auto [base_domain, base_range] = this->analyze(I.getBaseCollection());
  auto &base = I.getBaseCollection();
  auto [base_domain, base_range] = this->analyze(base);

  // Fetch the insertion point.
  auto [_, index_range] = this->analyze(I.getInsertionPoint(), true);

  // Construct the empty contents.
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(type_of(I)),
                      "AssocInsertInst with non-collection operand.");
  auto *element_type = &collection_type.getElementType();

  // If the element is a struct type, create an empty TupleContent.
  Content *empty_content = &Content::create<EmptyContent>();
  if (auto *struct_type = dyn_cast<TupleType>(element_type)) {
    // TODO: handle nested structs.
    vector<Content *> fields(struct_type->getNumFields(), empty_content);

    empty_content = &Content::create<TupleContent>(fields);
  }

  auto &domain = Content::create<UnionContent>(*index_range, *base_domain);

  auto &range = Content::create<UnionContent>(*empty_content, *base_range);

  return { &domain, &range };
}

namespace detail {
ContentSummary handle_collection_read(ReadInst &I, Content &collection_range) {
  // Construct a SubsetContent.
  auto &elem = Content::create<SubsetContent>(collection_range);

  // Wrap the element content in FieldContent(s) if necessary.
  Content *content = &elem;
  for (unsigned sub_dim = 0; sub_dim < I.getNumberOfSubIndices(); ++sub_dim) {
    // Get the sub index.
    auto sub_index = I.getSubIndex(sub_dim);

    // Wrap the current content in a FieldContent.
    content = &Content::create<FieldContent>(*content, sub_index);
  }

  return { &Content::create<UnderdefinedContent>(), content };
}
} // namespace detail

ContentSummary ContentAnalysisDriver::visitIndexReadInst(IndexReadInst &I) {
  // Construct the resultant content.
  auto [_, elements] = this->analyze(I.getObjectOperand());

  if (not ContentSimplification::is_simple(*elements)) {
    std::tie(_, elements) = detail::conservative(I.getObjectOperand());
  }

  return detail::handle_collection_read(I, *elements);
}

ContentSummary ContentAnalysisDriver::visitAssocReadInst(AssocReadInst &I) {
  // Construct the resultant content.
  auto [_, elements] = this->analyze(I.getObjectOperand());

  if (not ContentSimplification::is_simple(*elements)) {
    std::tie(_, elements) = detail::conservative(I.getObjectOperand());
  }

  return detail::handle_collection_read(I, *elements);
}

ContentSummary ContentAnalysisDriver::visitStructReadInst(StructReadInst &I) {
  // Get the struct content.
  auto [_, struct_range] = this->analyze(I.getObjectOperand());

  // Get the field index;
  auto field_index = I.getFieldIndex();

  // Create the FieldContent.
  auto &field_content =
      Content::create<FieldContent>(*struct_range, field_index);

  return { &Content::create<UnderdefinedContent>(), &field_content };
}

ContentSummary ContentAnalysisDriver::visitAssocKeysInst(AssocKeysInst &I) {
  // Construct the KeysContent
  auto [domain, _] = this->analyze(I.getCollection());
  return { &Content::create<RangeContent>(I.getCallInst()), domain };
}

ContentSummary ContentAnalysisDriver::contextualize_fold(FoldInst &I,
                                                         ContentSummary input) {

  // Fetch the content of the input collection.
  auto [folded_domain, folded_range] = this->analyze(I.getCollection());

  // Fetch the content of the initial value.
  auto [init_domain, init_range] = this->analyze(I.getInitial());

  // Fetch the content of the accumulator argument.
  auto [accum_arg_domain, accum_arg_range] =
      detail::conservative(I.getAccumulatorArgument());

  // Fetch the content of the index argument.
  auto &index_arg = Content::create<ScalarContent>(I.getIndexArgument());

  // Unpack the input.
  auto [input_domain, input_range] = input;

  // Substitute the function arguments with the operands.
  auto *domain = &input_domain->substitute(*accum_arg_domain, *init_domain)
                      .substitute(*accum_arg_range, *init_range)
                      .substitute(index_arg, *folded_domain);
  auto *range = &input_range->substitute(*accum_arg_domain, *init_domain)
                     .substitute(*accum_arg_range, *init_range)
                     .substitute(index_arg, *folded_domain);

  // If the element is non-void, fetch its content.
  if (auto *elem_arg = I.getElementArgument()) {
    auto &elem = Content::create<ScalarContent>(*elem_arg);
    domain = &domain->substitute(elem, *folded_range);
    range = &range->substitute(elem, *folded_range);
  }

  // Substitute each of the closed arguments.
  for (unsigned closed_idx = 0; closed_idx < I.getNumberOfClosed();
       ++closed_idx) {
    auto &closed_use = I.getClosedAsUse(closed_idx);
    auto &closed = *closed_use.get();
    auto &closed_arg = *I.getClosedArgument(closed_use);

    auto [closed_arg_domain, closed_arg_range] =
        detail::conservative(closed_arg);

    auto [closed_domain, closed_range] = this->analyze(closed);

    domain = &domain->substitute(*closed_arg_domain, *closed_domain)
                  .substitute(*closed_arg_range, *closed_range);
    range = &range->substitute(*closed_arg_domain, *closed_domain)
                 .substitute(*closed_arg_range, *closed_range);
  }

  return { domain, range };
}

ContentSummary ContentAnalysisDriver::visitFoldInst(FoldInst &I) {

  // Get the content of the returned accumulator.
  auto &fold_function = I.getFunction();
  llvm::Value *returned = nullptr;
  for (auto &BB : fold_function) {
    auto *terminator = BB.getTerminator();
    if (auto *return_inst = dyn_cast<llvm::ReturnInst>(terminator)) {

      // If we found two returns, error!
      if (returned != nullptr) {
        MEMOIR_UNREACHABLE("Fold function has multiple returns!");
      }

      returned = return_inst->getReturnValue();
    }
  }

  // Ensure that we found a single return value.
  if (not returned) {
    MEMOIR_UNREACHABLE("Could not find return in fold function!");
  }

  // Fetch the content of the returned collection.
  auto was = this->set_recurse();
  auto returned_summary = this->analyze(*returned);
  this->restore_recurse(was);

  // Contextualize the returned value.
  auto [domain, range] = this->contextualize_fold(I, returned_summary);

  // Substitute into the new scope.
  auto [old_var_domain, old_var_range] = detail::conservative(*returned);
  auto [new_var_domain, new_var_range] = detail::conservative(I.getResult());

  auto &subst_domain = domain->substitute(*old_var_domain, *new_var_domain)
                           .substitute(*old_var_range, *new_var_range);
  auto &subst_range = range->substitute(*old_var_domain, *new_var_domain)
                          .substitute(*old_var_range, *new_var_range);

  return { &subst_domain, &subst_range };
}

ContentSummary ContentAnalysisDriver::visitClearInst(ClearInst &I) {
  return { &Content::create<EmptyContent>(), &Content::create<EmptyContent>() };
}

ContentSummary ContentAnalysisDriver::visitUsePHIInst(UsePHIInst &I) {
  // Propagate the used collection.
  return this->analyze(I.getUsed());
}

ContentSummary ContentAnalysisDriver::visitPHINode(llvm::PHINode &I) {

  // If this PHI is a GAMMA, create a union of two ConditionalContents.
  llvm::BasicBlock *if_block, *else_block;
  if (auto *branch =
          llvm::GetIfCondition(I.getParent(), if_block, else_block)) {

    // Create the content for the TRUE branch.
    auto *true_value = I.getIncomingValueForBlock(if_block);
    auto [true_domain, true_range] = this->analyze(*true_value);

    // Create the content for the FALSE branch.
    auto *false_value = I.getIncomingValueForBlock(else_block);
    auto [false_domain, false_range] = this->analyze(*false_value);

    // Unpack the conditional branch.
    llvm::CmpInst::Predicate pred;
    llvm::Value *lhs;
    llvm::Value *rhs;

    auto *cond = branch->getCondition();
    if (auto *cmp = dyn_cast<llvm::CmpInst>(cond)) {
      // If the conditional is a compare instruction, unpack it.
      pred = cmp->getPredicate();
      lhs = cmp->getOperand(0);
      rhs = cmp->getOperand(1);

    } else {
      // Otherwise, construct a check that the value is TRUE.
      pred = llvm::CmpInst::Predicate::ICMP_EQ;
      lhs = cond;
      rhs = llvm::ConstantInt::getTrue(cond->getType());
    }

    // Construct the CollectionContents
    auto [_lhs_domain, lhs_content] = this->analyze(*lhs);
    auto [_rhs_domain, rhs_content] = this->analyze(*rhs);

    // Construct the ConditionalContents.
    auto &if_domain = Content::create<ConditionalContent>(*true_domain,
                                                          pred,
                                                          *lhs_content,
                                                          *rhs_content);
    auto &if_range = Content::create<ConditionalContent>(*true_range,
                                                         pred,
                                                         *lhs_content,
                                                         *rhs_content);
    auto &else_domain = Content::create<ConditionalContent>(
        *false_domain,
        llvm::CmpInst::getInversePredicate(pred),
        *lhs_content,
        *rhs_content);
    auto &else_range = Content::create<ConditionalContent>(
        *false_range,
        llvm::CmpInst::getInversePredicate(pred),
        *lhs_content,
        *rhs_content);

    // Construct the UnionContent.
    auto &domain = Content::create<UnionContent>(if_domain, else_domain);
    auto &range = Content::create<UnionContent>(if_range, else_range);

    return { &domain, &range };
  }

  // See if the PHI is a MU.
  if (auto *loop = this->get_loop_for(I)) {

    // Fetch the incoming and back edges of the loop.
    llvm::BasicBlock *incoming, *backedge;
    loop->getIncomingAndBackEdge(incoming, backedge);

    // Get the initial and recurrent value of the MU.
    auto *initial = I.getIncomingValueForBlock(incoming);
    auto *recurrent = I.getIncomingValueForBlock(backedge);

    // Analyze the incoming values.
    auto [phi_domain, phi_range] = detail::conservative(I);
    auto [initial_domain, initial_range] = this->analyze(*initial);
    auto [recur_var_domain, recur_var_range] = detail::conservative(*recurrent);
    auto [recur_domain, recur_range] = this->analyze(*recurrent);

    // Substitute any uses of the PHI with the initial contents.
    auto &domain = recur_domain->substitute(*phi_domain, *initial_domain)
                       .substitute(*phi_range, *initial_range)
                       .substitute(*recur_var_domain, *initial_domain)
                       .substitute(*recur_var_range, *initial_range);
    auto &range = recur_range->substitute(*phi_domain, *initial_domain)
                      .substitute(*phi_range, *initial_range)
                      .substitute(*recur_var_domain, *initial_domain)
                      .substitute(*recur_var_range, *initial_range);

    return { &domain, &range };
  }

  // See if the PHI is an ETA.
  if (false and I.getNumIncomingValues() == 1) {
    auto *incoming_block = I.getIncomingBlock(0);
    auto &incoming_terminator =
        MEMOIR_SANITIZE(incoming_block->getTerminator(),
                        "No terminator in incoming block!");

    // Check if the single incoming edge is a loop exit.
    if (auto *incoming_loop = this->get_loop_for(incoming_terminator)) {

      // If the incoming block is the loop exit block, then this is an ETA.
      // NOTE: For now we assume that all loops have a single exit block.
      if (incoming_loop->isLoopExiting(incoming_block)) {
        // Don't recurse on the incoming values.
        // TODO: This should be changed to recurse and perform the replacements
        // as specified in the paper.
        return detail::conservative(I);
      }
    }
  }

  // Otherwise, return a conservative content.
  set<llvm::Value *> incoming_values = {};
  vector<ContentSummary> incoming_contents = {};
  incoming_contents.reserve(I.getNumIncomingValues());

  for (unsigned i = 0; i < I.getNumIncomingValues(); ++i) {
    auto *incoming_value = I.getIncomingValue(i);
    if (incoming_values.count(incoming_value) > 0) {
      continue;
    } else {
      incoming_values.insert(incoming_value);
    }

    auto was = this->set_recurse();
    incoming_contents.push_back(this->analyze(*incoming_value));
    this->restore_recurse(was);
  }

  auto content = std::accumulate(
      std::next(incoming_contents.rbegin()),
      incoming_contents.rend(),
      incoming_contents.back(),
      [](ContentSummary accum, ContentSummary incoming) -> ContentSummary {
        auto [accum_domain, accum_range] = accum;
        auto [incoming_domain, incoming_range] = incoming;
        auto &domain =
            Content::create<UnionContent>(*incoming_domain, *accum_domain);
        auto &range =
            Content::create<UnionContent>(*incoming_range, *accum_range);
        return { &domain, &range };
      });

  return content;
}

namespace detail {
bool value_is_local(llvm::Value &V, llvm::Function &F) {
  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return &F == arg->getParent();
  } else if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return &F == inst->getParent()->getParent();
  }

  return false;
}

bool contents_are_local(Content &C, llvm::Function &F) {
  if (auto *keys = dyn_cast<KeysContent>(&C)) {
    return value_is_local(keys->collection(), F);
  } else if (auto *elems = dyn_cast<ElementsContent>(&C)) {
    return value_is_local(elems->collection(), F);
  } else if (auto *subset = dyn_cast<SubsetContent>(&C)) {
    return contents_are_local(subset->content(), F);
  } else if (auto *scalar = dyn_cast<ScalarContent>(&C)) {
    return value_is_local(scalar->value(), F);
  } else if (auto *field = dyn_cast<FieldContent>(&C)) {
    return contents_are_local(field->parent(), F);
  } else if (auto *cond = dyn_cast<ConditionalContent>(&C)) {
    return contents_are_local(cond->lhs(), F)
           and contents_are_local(cond->rhs(), F)
           and contents_are_local(cond->content(), F);
  } else if (auto *tuple = dyn_cast<TupleContent>(&C)) {
    auto &elements = tuple->elements();
    return std::accumulate(elements.begin(),
                           elements.end(),
                           true,
                           [&F](bool is_local, Content *child) -> bool {
                             return is_local and contents_are_local(*child, F);
                           });
  } else if (auto *union_ = dyn_cast<UnionContent>(&C)) {
    return contents_are_local(union_->lhs(), F)
           and contents_are_local(union_->rhs(), F);
  }

  return false;
}

} // namespace detail

ContentSummary ContentAnalysisDriver::contextualize_call(
    llvm::CallBase &call,
    llvm::Function &function,
    ContentSummary content) {

  // If the call is in the function, do nothing.
  if (call.getFunction() == &function) {
    return content;
  }

  // Unpack the content.
  auto [domain, range] = content;

  // Collect all live out variables.
  map<llvm::Argument *, llvm::Value *> live_outs = {};
  for (auto &BB : function) {
    for (auto &I : BB) {
      auto live_out_metadata = Metadata::get<LiveOutMetadata>(I);
      if (not live_out_metadata) {
        continue;
      }

      auto arg_no = live_out_metadata->getArgNo();
      auto *argument = function.getArg(arg_no);

      live_outs[argument] = &I;
    }
  }

  // For each live-out, find a the corresponding RET-PHI.
  map<llvm::Argument *, llvm::Value *> ret_phis = {};
  llvm::BasicBlock::iterator it(call.getNextNode());
  for (auto *inst = call.getNextNode(); inst != nullptr;
       inst = inst->getNextNode()) {

    if (auto *ret_phi = into<RetPHIInst>(inst)) {
      // Determine which argument this ret phi corresponds to.
      auto &input = ret_phi->getInput();
      bool found = false;
      unsigned operand_no = 0;
      for (auto &operand_use : call.args()) {
        if (operand_use.get() == &input) {
          operand_no = operand_use.getOperandNo();
          found = true;
          break;
        }
      }
      MEMOIR_ASSERT(found, "Could not find corresponding operand");

      auto *argument = function.getArg(operand_no);
      ret_phis[argument] = inst;

    } else if (auto *call = dyn_cast<llvm::CallBase>(inst)) {
      break;
    }
  }

  // For each argument of the function, contextualize it.
  for (auto &argument : function.args()) {
    // Fetch the conservative argument content.
    auto [arg_domain, arg_range] = detail::conservative(argument);

    // Analyze the incoming argument operand.
    auto &operand = MEMOIR_SANITIZE(call.getArgOperand(argument.getArgNo()),
                                    "Argument operand for CallBase is NULL");

    auto was = this->set_recurse();
    auto [operand_domain, operand_range] = this->analyze(operand);
    this->restore_recurse(was);

    // Substitute the operand for the argument.
    domain = &domain->substitute(*arg_domain, *operand_domain)
                  .substitute(*arg_range, *operand_range);
    range = &domain->substitute(*arg_range, *operand_domain)
                 .substitute(*arg_range, *operand_range);

    // If there is a live out, substitute it with the RET-PHI.
    auto found_live_out = live_outs.find(&argument);
    if (found_live_out != live_outs.end()) {
      auto *live_out = found_live_out->second;

      auto found_ret_phi = ret_phis.find(&argument);
      if (found_ret_phi != ret_phis.end()) {
        auto *ret_phi = found_ret_phi->second;

        auto [live_out_domain, live_out_range] =
            detail::conservative(*live_out);
        auto [ret_phi_domain, ret_phi_range] = detail::conservative(*ret_phi);

        // Substitute.
        domain = &domain->substitute(*live_out_domain, *ret_phi_domain)
                      .substitute(*live_out_range, *ret_phi_range);
        range = &domain->substitute(*live_out_range, *ret_phi_domain)
                     .substitute(*live_out_range, *ret_phi_range);
      }
    }
  }

  return { domain, range };
}

ContentSummary ContentAnalysisDriver::visitRetPHIInst(RetPHIInst &I) {

  // Find the corresponding call.
  auto &called = I.getCalledOperand();
  llvm::CallBase *call = nullptr;
  auto *prev = I.getCallInst().getPrevNonDebugInstruction();
  while (prev) {
    // Skip non-call instructions.
    if (auto *prev_call = dyn_cast<llvm::CallBase>(prev)) {
      // If the called operand is the same as the RetPHI, we found it.
      if (prev_call->getCalledOperand() == &called) {
        call = prev_call;
        break;
      }
    }

    // Otherwise, iterate.
    prev = prev->getPrevNonDebugInstruction();
  }
  if (not call) {
    MEMOIR_UNREACHABLE(
        "Failed to find call instruction corresponding to the RetPHI.");
  }

  // Handle FoldInst.
  if (auto *fold = into<FoldInst>(call)) {

    // Determine the argument number.
    bool found_arg = false;
    unsigned arg_number = -1;
    for (auto &use : call->args()) {
      if (use.get() == &I.getInput()) {
        arg_number = use.getOperandNo();
        found_arg = true;
        break;
      }
    }
    if (not found_arg) {
      MEMOIR_UNREACHABLE("Failed to find corresponding argument.");
    }

    // Get the corresponding use.
    auto &operand_use = call->getArgOperandUse(arg_number);

    // Get the corresponding argument.
    auto &closed_argument = *fold->getClosedArgument(operand_use);

    // Get the corresponding live-out content.
    llvm::Value *live_out = nullptr;
    for (auto &inst : llvm::instructions(fold->getFunction())) {
      auto metadata = llvm::memoir::Metadata::get<LiveOutMetadata>(inst);
      if (not metadata.has_value()) {
        continue;
      }

      if (metadata->getArgNo() == closed_argument.getArgNo()) {
        live_out = &inst;
        break;
      }
    }

    // If we did not find a live out, then the contents are the same
    // as the input.
    if (not live_out) {
      return this->analyze(I.getInput());
    }

    // Analyze the live out
    auto was = this->set_recurse();
    auto live_out_summary = this->analyze(*live_out);
    this->restore_recurse(was);

    // Otherwise, get the content of the live out.
    auto [domain, range] = this->contextualize_fold(*fold, live_out_summary);

    // Substitute into the new scope.
    auto [old_var_domain, old_var_range] = detail::conservative(*live_out);
    auto [new_var_domain, new_var_range] = detail::conservative(I);
    return { &domain->substitute(*old_var_domain, *new_var_domain),
             &range->substitute(*old_var_range, *new_var_range) };

  }

  // Handle direct calls.
  else if (auto *called_function = I.getCalledFunction()) {
    // Determine the argument number.
    bool found_arg = false;
    unsigned arg_number = -1;
    for (auto &use : call->args()) {
      if (use.get() == &I.getInput()) {
        arg_number = use.getOperandNo();
        found_arg = true;
        break;
      }
    }
    if (not found_arg) {
      MEMOIR_UNREACHABLE("Failed to find corresponding argument.");
    }

    // For a direct call, get the corresponding live-out content.
    llvm::Value *live_out = nullptr;
    for (auto &inst : llvm::instructions(*called_function)) {
      auto metadata = llvm::memoir::Metadata::get<LiveOutMetadata>(inst);
      if (not metadata.has_value()) {
        continue;
      }

      if (metadata->getArgNo() == arg_number) {
        live_out = &inst;
        break;
      }
    }

    // If we did not find a live out, then the contents are the same
    // as the input.
    if (not live_out) {
      debugln("No live out: ", I);
      debugln("Argument #: ", arg_number);
      return this->analyze(I.getInput());
    }

    // Analyze the live out.
    ContentSummary live_out_summary;
    if (call->getFunction() != called_function) {
      auto was = this->set_recurse();
      live_out_summary = this->analyze(*live_out);
      this->restore_recurse(was);
    } else {
      live_out_summary = this->analyze(*live_out);
    }

    // Contextualize the contents in the context of the call.
    auto [live_out_domain, live_out_range] =
        this->contextualize_call(*call, *called_function, live_out_summary);

    // If there are out-of-scope values in the resulting contents, we will
    // just return the conservative tautology.
    auto &parent_function = MEMOIR_SANITIZE(I.getCallInst().getFunction(),
                                            "RetPHI has no parent function");
    if (false
        and not detail::contents_are_local(*live_out_domain, parent_function)) {
      println("Content is not local!");
      println("  ", *live_out_domain);

      auto [new_var_domain, _] = detail::conservative(I);
      live_out_domain = new_var_domain;
    }

    if (false
        and not detail::contents_are_local(*live_out_range, parent_function)) {
      auto [_, new_var_range] = detail::conservative(I);
      live_out_range = new_var_range;
    }

    return { live_out_domain, live_out_range };
  }

  // For an indirect call, we will conservatively say that we know
  // nothing.
  return detail::conservative(I.getCallInst());
}

void ContentAnalysisDriver::initialize() {

  // First, create the initial summaries based solely on the local instruction
  // information.
  for (auto &F : this->M) {

    if (F.empty()) {
      continue;
    }

    // Check if this function is the body of a fold.
    llvm::Instruction *fold_context = nullptr;
    if (auto *single_use = F.getSingleUndroppableUse()) {
      if (auto *fold_user = into<FoldInst>(single_use->getUser())) {
        fold_context = &fold_user->getCallInst();
      }
    }

    for (auto &A : F.args()) {
      this->current =
          fold_context ? fold_context : F.getEntryBlock().getFirstNonPHI();
      this->visited.clear();
      this->summarize(A, this->visitArgument(A));
    }

    for (auto &BB : F) {
      for (auto &I : BB) {

        // Skip non-collections.
        auto *type = type_of(I);
        if (not isa_and_nonnull<SequenceType>(type)
            and not isa_and_nonnull<AssocArrayType>(type)) {
          continue;
        }

        this->current = fold_context ? fold_context : &I;
        this->visited.clear();
        this->summarize(I, this->analyze(I));
      }
    }
  }

// Print the initial contents.
#if 0
  debugln();
  debugln("================================");
  debugln("Initial contents:");
  for (const auto &[value, contents] : this->result) {
    debugln(*value);
    debugln("domain(",
            value_name(*value),
            ") contains ",
            *contents.first);
    debugln("range(",
            value_name(*value),
            ") contains ",
            *contents.second);
    debugln();
  }
  debugln("================================");
  debugln();
#endif

  return;
}
// =========================================================================

// =========================================================================
// Simplification

void ContentAnalysisDriver::simplify() {

  ContentSimplification simplifier(this->result);

  int timeout = 10;
  for (int i = 0; i < timeout; ++i) {

    // For each content mapping, simplify it.
    for (auto &[value, content] : this->result) {

      // Simplify the content for the given value mapping.
      auto &[domain, range] = content;

      domain = &simplifier.simplify(*domain);

      range = &simplifier.simplify(*range);

      // If the value is a ret-phi, contextualize it.
      // if (auto *ret_phi = into<RetPHIInst>(*value)) {
      //   if (auto *function = ret_phi->getCalledFunction()) {

      //     if (not FunctionNames::is_memoir_call(*function)) {

      //       // Find the call.
      //       llvm::CallBase *corresponding_call = nullptr;
      //       llvm::Instruction *current_inst = &ret_phi->getCallInst();
      //       while ((current_inst = current_inst->getPrevNode())) {
      //         if (auto *ret_phi = into<RetPHIInst>(current_inst)) {
      //           continue;
      //         }

      //         if (auto *call = dyn_cast<llvm::CallBase>(current_inst)) {
      //           if (call->getCalledFunction() == function) {
      //             corresponding_call = call;
      //             break;
      //           } else {
      //             break;
      //           }
      //         }
      //       }

      //       if (corresponding_call) {
      //         std::tie(domain, range) =
      //             this->contextualize_call(*corresponding_call,
      //                                      *function,
      //                                      std::make_pair(domain, range));
      //       }
      //     }
      //   }
      // }
    }
  }

  // Print the initial contents.
  debugln();
  debugln("================================");
  debugln("Simplified contents:");
  for (auto &F : M) {

    if (F.empty()) {
      continue;
    }

    auto first = true;
    for (const auto &[value, contents] : this->result) {

      if (auto *inst = dyn_cast<llvm::Instruction>(value)) {
        if (inst->getFunction() != &F) {
          continue;
        }
      } else if (auto *arg = dyn_cast<llvm::Argument>(value)) {
        if (arg->getParent() != &F) {
          continue;
        }
      }

      if (not isa_and_nonnull<CollectionType>(type_of(*value))) {
        continue;
      }

      if (first) {
        debugln("=== BEGIN ", F.getName(), "===");
        first = false;
      }

      debugln("domain(", value_name(*value), ") contains ", *contents.first);
      debugln("range(", value_name(*value), ") contains ", *contents.second);
    }
    if (not first) {
      debugln("===   END ", F.getName(), "===");
    }
  }
  debugln("================================");
  debugln();

  // All done.
  return;
}
// =========================================================================

ContentAnalysisDriver::ContentAnalysisDriver(
    Contents &result,
    llvm::Module &M,
    std::function<llvm::Loop *(llvm::Instruction &)> get_loop_for)
  : result(result),
    M(M),
    recurse(false),
    get_loop_for(get_loop_for) {

  // Gather the initial contents with a per-instruction analysis.
  this->initialize();

  // Simplify the results to be in a canonical form.
  this->simplify();
}

Contents ContentAnalysis::run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
  Contents result;

  // Create a closure to access the loop for the given PHI.
  auto get_loop_for = [&](llvm::Instruction &I) -> llvm::Loop * {
    if (auto *parent = I.getParent()) {
      if (auto *function = parent->getParent()) {
        // Fetch the loop info.
        auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);
        auto &loop_info = FAM.getResult<llvm::LoopAnalysis>(*function);

        // Fetch the loop, if it exists.
        if (auto *loop = loop_info.getLoopFor(parent)) {

          // If the loop's header is the PHI's parent, we found the loop!
          if (loop->getHeader() == parent) {
            return loop;
          }
        }
      }
    }

    return nullptr;
  };

  // Construct and run content analysis.
  ContentAnalysisDriver(result, M, get_loop_for);

  // Return the analyzed contents.
  return result;
}

llvm::AnalysisKey ContentAnalysis::Key;

} // namespace folio
