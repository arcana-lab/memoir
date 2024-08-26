#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/TypeCheck.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/Metadata.hpp"

#include "folio/analysis/ContentAnalysis.hpp"

using namespace llvm::memoir;

namespace folio {

// =========================================================================
// Initialization
namespace detail {

using namespace llvm::memoir;

Content &conservative(llvm::Value &V) {
  auto *type = type_of(V);

  if (isa_and_nonnull<SequenceType>(type)
      or isa_and_nonnull<AssocArrayType>(type)) {
    auto &content = Content::create<CollectionContent>(V);
    return content;
  } else if (isa_and_nonnull<StructType>(type)) {
    auto &content = Content::create<StructContent>(V);
    return content;
  } else {
    auto &content = Content::create<ScalarContent>(V);
    return content;
  }
}

Content &conservative(MemOIRInst &I) {
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

ContentSummary ContentAnalysisDriver::analyze(llvm::Value &V) {

  // Check if we have already analyzed this value.
  auto found = this->result.find(&V);
  if (found != this->result.end()) {
    return found->second;
  }

  // Check if we have already visited this value.
  if (visited.count(&V) > 0) {
    // Already visited, return the conservative result.
    return { &detail::conservative(V), &detail::conservative(V) };
  } else {
    visited.insert(&V);
  }

  // If we haven't, dispatch to the appropriate handler.
  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    auto content = this->visitArgument(*arg);
    this->summarize(V, content);
    return content;
  } else if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    auto content = this->visit(*inst);
    this->summarize(V, content);
    return content;
  }

  // If the value isn't handled by any of our handlers, output a conservative
  // result.
  auto &content = Content::create<ScalarContent>(V);
  return { &content, &content };
}

ContentSummary ContentAnalysisDriver::analyze(MemOIRInst &I) {
  return this->analyze(I.getCallInst());
}

ContentSummary ContentAnalysisDriver::visitArgument(llvm::Argument &A) {
  return { &detail::conservative(A), &detail::conservative(A) };
}

ContentSummary ContentAnalysisDriver::visitInstruction(llvm::Instruction &I) {
  return { &detail::conservative(I), &detail::conservative(I) };
}

ContentSummary ContentAnalysisDriver::visitSequenceAllocInst(
    SequenceAllocInst &I) {
  auto &empty = Content::create<EmptyContent>();

  return { &detail::conservative(I), &empty };
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
  auto [_, value_range] = this->analyze(I.getInsertionPoint());

  // Create the domain union.
  auto &domain = Content::create<UnionContent>(*value_range, *base_domain);

  // Analyze the collection type.
  auto *type = type_of(I.getResultCollection());
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(type),
                      "SeqInsertInst on non-collection type");
  auto *struct_type = dyn_cast<StructType>(&collection_type.getElementType());

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

  // Fetch the content of the input collection.
  auto [base_domain, base_range] = this->analyze(I.getObjectOperand());

  // Analyze the index being written.
  auto [_index_domain, index_range] = this->analyze(I.getIndex());

  // Create the domain.
  auto &domain = Content::create<UnionContent>(*index_range, *base_domain);

  // Analyze the value being written.
  auto [_value_domain, value_range] = this->analyze(I.getValueWritten());

  // Get the type information.
  auto &collection_type = I.getCollectionType();
  auto *element_type = &collection_type.getElementType();

  // Handle subindices.
  auto *element_content = value_range;
  for (auto sub_dim = 0; sub_dim < I.getNumberOfSubIndices(); ++sub_dim) {

    // Get the struct type.
    auto &struct_type = MEMOIR_SANITIZE(dyn_cast<StructType>(element_type),
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

  return { &domain, &range };
}

ContentSummary ContentAnalysisDriver::visitSeqInsertValueInst(
    SeqInsertValueInst &I) {

  // Fetch the base collection info.
  auto [base_domain, base_range] = this->analyze(I.getBaseCollection());

  // Fetch the insertion point info.
  auto [_index_domain, index_range] = this->analyze(I.getInsertionPoint());

  // Fetch the inserted value's range.
  auto [_value_domain, value_range] = this->analyze(I.getValueInserted());

  // Construct the new domain.
  auto &domain = Content::create<UnionContent>(*base_domain, *index_range);

  // Construct the new range.
  auto &range = Content::create<UnionContent>(*base_range, *value_range);

  return { &domain, &range };
}

ContentSummary ContentAnalysisDriver::visitAssocWriteInst(AssocWriteInst &I) {

  // Fetch the base collection info.
  auto [base_domain, base_range] = this->analyze(I.getObjectOperand());

  // Fetch the inserted value's range.
  auto [_, value_range] = this->analyze(I.getValueWritten());

  // Construct the new domain.
  auto &domain = *base_domain;

  // Construct the new range.
  auto &range = Content::create<UnionContent>(*value_range, *base_range);

  return { &domain, &range };
}

ContentSummary ContentAnalysisDriver::visitAssocInsertInst(AssocInsertInst &I) {

  // Fetch the base collection info.
  auto [base_domain, base_range] = this->analyze(I.getBaseCollection());

  // Fetch the insertion point.
  auto [_index_domain, index_range] = this->analyze(I.getInsertionPoint());

  // Construct the empty contents.
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(type_of(I)),
                      "AssocInsertInst with non-collection operand.");
  auto *element_type = &collection_type.getElementType();

  // If the element is a struct type, create an empty TupleContent.
  Content *empty_content = &Content::create<EmptyContent>();
  if (auto *struct_type = dyn_cast<StructType>(element_type)) {
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
  // Construct an ElemContent.
  auto &elem = Content::create<ElementContent>(collection_range);

  // Wrap the element content in FieldContent(s) if necessary.
  Content *content = &elem;
  for (auto sub_dim = 0; sub_dim < I.getNumberOfSubIndices(); ++sub_dim) {
    // Get the sub index.
    auto sub_index = I.getSubIndex(sub_dim);

    // Wrap the current content in a FieldContent.
    content = &Content::create<FieldContent>(*content, sub_index);
  }

  return { &Content::create<UnderdefinedContent>(), content };
}
} // namespace detail

ContentSummary ContentAnalysisDriver::visitIndexReadInst(IndexReadInst &I) {
  // Analyze the collection.
  auto [_, range] = this->analyze(I.getObjectOperand());

  // Construct the resultant content.
  return detail::handle_collection_read(I, *range);
}

ContentSummary ContentAnalysisDriver::visitAssocReadInst(AssocReadInst &I) {
  // Analyze the collection.
  auto [_, range] = this->analyze(I.getObjectOperand());

  // Construct the resultant content.
  return detail::handle_collection_read(I, *range);
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
  // Analyze the collection.
  auto [domain, _] = this->analyze(I.getCollection());

  // Construct the KeyContent
  return { &detail::conservative(I), &Content::create<KeysContent>(*domain) };
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

  // Fetch the content of the returned value.
  auto [returned_domain, returned_range] = this->analyze(*returned);

  // Fetch the content of the input collection.
  auto [collection_domain, collection_range] = this->analyze(I.getCollection());

  // Fetch the content of the initial value.
  auto [init_domain, init_range] = this->analyze(I.getInitial());

  // Substitute the function arguments with the operands.
  auto *accum_domain =
      &returned_domain->substitute(I.getAccumulatorArgument(), *init_domain)
           .substitute(I.getIndexArgument(),
                       Content::create<KeysContent>(*collection_domain));
  auto *accum_range =
      &returned_range->substitute(I.getAccumulatorArgument(), *init_domain)
           .substitute(I.getIndexArgument(),
                       Content::create<KeysContent>(*collection_domain));

  // If the element is non-void, fetch its content.
  if (auto *elem_arg = I.getElementArgument()) {
    accum_domain = &accum_domain->substitute(
        *elem_arg,
        Content::create<ElementsContent>(*collection_range));
    accum_range = &accum_range->substitute(
        *elem_arg,
        Content::create<ElementsContent>(*collection_range));
  }

  // Construct the union'd content of the initial and accumulated.
  auto &domain = Content::create<UnionContent>(*accum_domain, *init_domain);
  auto &range = Content::create<UnionContent>(*accum_range, *init_range);

  return { &domain, &range };
}

ContentSummary ContentAnalysisDriver::visitUsePHIInst(UsePHIInst &I) {
  // Propagate the used collection.
  return this->analyze(I.getUsedCollection());
}

ContentSummary ContentAnalysisDriver::visitPHINode(llvm::PHINode &I) {
  // If this PHI is a copy, propagate the single input.
  if (I.getNumIncomingValues() == 1) {
    auto *incoming_value = I.getIncomingValue(0);
    return this->analyze(*incoming_value);
  }

  // If this PHI is a GAMMA, create a union of two ConditionalContents.
  llvm::BasicBlock *if_block, *else_block;
  auto *branch = llvm::GetIfCondition(I.getParent(), if_block, else_block);
  if (branch) {

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

  // Otherwise, return a conservative content.
  vector<ContentSummary> incoming_contents = {};
  incoming_contents.reserve(I.getNumIncomingValues());

  for (auto i = 0; i < I.getNumIncomingValues(); ++i) {
    auto *incoming_value = I.getIncomingValue(i);
    incoming_contents.push_back(this->analyze(*incoming_value));
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
      if (use.get() == &I.getInputCollection()) {
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
    auto &closed_argument = fold->getClosedArgument(operand_use);

    // Get the corresponding live-out content.
    llvm::Value *live_out = nullptr;
    for (auto &inst : llvm::instructions(fold->getFunction())) {
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
      return this->analyze(closed_argument);
    }

    // Otherwise, get the content of the live out.
    auto [live_out_domain, live_out_range] = this->analyze(*live_out);

    // Fetch the content of the initial value.
    auto [input_domain, input_range] = this->analyze(I.getInputCollection());

    // Fetch the content of the collection being folded over.
    auto [folded_domain, folded_range] = this->analyze(fold->getCollection());

    // Substitute the function arguments with the operands.
    auto *domain =
        &live_out_domain->substitute(closed_argument, *input_domain)
             .substitute(fold->getIndexArgument(),
                         Content::create<KeysContent>(*folded_domain));
    auto *range = &live_out_range->substitute(closed_argument, *input_range)
                       .substitute(fold->getIndexArgument(),
                                   Content::create<KeysContent>(*folded_range));

    // If the element is non-void, fetch its content.
    if (auto *elem_arg = fold->getElementArgument()) {
      domain =
          &domain->substitute(*elem_arg,
                              Content::create<ElementsContent>(*folded_range));
      range =
          &range->substitute(*elem_arg,
                             Content::create<ElementsContent>(*folded_range));
    }

    return { &Content::create<UnionContent>(*domain, *input_domain),
             &Content::create<UnionContent>(*range, *input_range) };
  }

  // Handle direct calls.
  else if (auto *called_function = I.getCalledFunction()) {
    // Determine the argument number.
    bool found_arg = false;
    unsigned arg_number = -1;
    for (auto &use : call->args()) {
      if (use.get() == &I.getInputCollection()) {
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
      return this->analyze(I.getInputCollection());
    }

    // Otherwise, get the content of the live out.
    auto [live_out_domain, live_out_range] = this->analyze(*live_out);

    // Substitute the argument for the operand.
    auto &operand = MEMOIR_SANITIZE(call->getArgOperand(arg_number),
                                    "Argument operand for CallBase is NULL");
    auto [operand_domain, operand_range] = this->analyze(operand);
    auto &argument = MEMOIR_SANITIZE(called_function->getArg(arg_number),
                                     "Argument is NULL");
    auto &subst_domain = live_out_domain->substitute(argument, *operand_domain);
    auto &subst_range = live_out_range->substitute(argument, *operand_range);

    return { &subst_domain, &subst_range };
  }

  // For an indirect call, we will conservatively say that we know
  // nothing.
  auto &content = detail::conservative(I.getCallInst());

  return { &content, &content };
}

void ContentAnalysisDriver::initialize() {

  // Iterate over the CFG.
  vector<llvm::BasicBlock *> worklist;

  // First, create the initial summaries based solely on the local instruction
  // information.
  for (auto &F : this->M) {
    for (auto &BB : F) {
      for (auto &I : BB) {

        // Skip non-collections.
        auto *type = type_of(I);
        if (not isa_and_nonnull<SequenceType>(type)
            and not isa_and_nonnull<AssocArrayType>(type)) {
          continue;
        }

        this->analyze(I);
      }
    }
  }

  // Print the initial contents.
  println();
  println("================================");
  println("Initial contents:");
  for (const auto &[value, contents] : this->result) {
    println("domain(",
            value_name(*value),
            ") contains ",
            contents.first->to_string());
    println("range(",
            value_name(*value),
            ") contains ",
            contents.second->to_string());
  }
  println("================================");
  println();

  return;
}
// =========================================================================

// =========================================================================
// Simplification
namespace detail {

Content &simplify(llvm::Value &V, Content &C);

Content &simplify_conditional(llvm::Value &V, ConditionalContent &C) {
  // Recurse.
  auto &content = simplify(V, C.content());

  // If recursion simplified the inner content, update ourselves.
  if (&content != &C.content()) {
    return Content::create<ConditionalContent>(content,
                                               C.predicate(),
                                               C.lhs(),
                                               C.rhs());
  }

  return C;
}

Content &simplify_indexed(llvm::Value &V, IndexedContent &C) {
  auto &index = simplify(V, C.index());
  auto &element = simplify(V, C.element());

  if (&index != &C.index() or &element != &C.element()) {
    return Content::create<IndexedContent>(index, element);
  }

  return C;
}

Content &simplify_union(llvm::Value &V, UnionContent &C) {

  println("simplify ", C.to_string());

  // Unpack the union.
  auto &lhs = simplify(V, C.lhs());
  auto &rhs = simplify(V, C.rhs());

  println("  simplified lhs ", lhs.to_string());
  println("  simplified rhs ", rhs.to_string());
  println();

  // C U empty = C
  if (isa<EmptyContent>(&rhs)) {
    return lhs;
  }

  // empty U C = C
  if (isa<EmptyContent>(&lhs)) {
    return rhs;
  }

  // Fixed-point simplification
  // V :- C U V = V
  if (auto *rhs_collection = dyn_cast<CollectionContent>(&rhs)) {
    if (&rhs_collection->collection() == &V) {
      return lhs;
    }
  }

  // [ ..., x, ... ] U [ ..., empty, ... ] = [ ..., x, ... ]
  auto *lhs_tuple = dyn_cast<TupleContent>(&lhs);
  auto *rhs_tuple = dyn_cast<TupleContent>(&rhs);
  if (lhs_tuple and rhs_tuple) {
    // For each element, merge empty contents.
    auto &lhs_elements = lhs_tuple->elements();
    auto &rhs_elements = rhs_tuple->elements();

    MEMOIR_ASSERT(lhs_elements.size() == rhs_elements.size(),
                  "TupleContents being union'd are not same size.");

    auto size = lhs_elements.size();

    llvm::memoir::vector<Content *> new_elements = {};
    new_elements.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      auto &lhs_elem = simplify(V, lhs_tuple->element(i));
      auto &rhs_elem = simplify(V, rhs_tuple->element(i));

      // TODO: simplify lhs_elem and rhs_elem

      if (isa<EmptyContent>(&lhs_elem)) {
        new_elements.push_back(&rhs_elem);
      } else if (isa<EmptyContent>(&rhs_elem)) {
        new_elements.push_back(&lhs_elem);
      } else if (lhs_elem == rhs_elem) {
        new_elements.push_back(&lhs_elem);
      } else {
        // If there is a collision, we cannot simplify to a single tuple.
        return C;
      }
    }

    // Construct a new tuple.
    auto &new_tuple = Content::create<TupleContent>(new_elements);

    return new_tuple;
  }

  // (x | c1) U (empty | c2) = (x | c1)
  auto *lhs_cond = dyn_cast<ConditionalContent>(&lhs);
  auto *rhs_cond = dyn_cast<ConditionalContent>(&rhs);
  if (lhs_cond and rhs_cond) {
    auto &lhs_content = lhs_cond->content();
    auto &rhs_content = rhs_cond->content();

    if (isa<EmptyContent>(&lhs_content)) {
      return *rhs_cond;
    }

    if (isa<EmptyContent>(&rhs_content)) {
      return *lhs_cond;
    }
  }

  // If nothing happened, then see if we can reassociate the union and go again.
  if (auto *rhs_union = dyn_cast<UnionContent>(&rhs)) {
    auto &rhs_lhs = rhs_union->lhs();
    auto &rhs_rhs = rhs_union->rhs();
    auto &new_union = simplify(V, Content::create<UnionContent>(lhs, rhs_lhs));

    return simplify(V, Content::create<UnionContent>(new_union, rhs_rhs));
  }

  // If we weren't able to reassociate, create a new union content if any
  // sub-simplifications happened.
  if (&lhs != &C.lhs() or &rhs != &C.rhs()) {
    return simplify(V, Content::create<UnionContent>(lhs, rhs));
  }

  return C;
}

// TODO
Content &simplify(llvm::Value &V, Content &C) {
  // Dispatch to the appropriate content.
  // NOTE: this could be replaced with a ContentVisitor, but I don't want to
  // implement one unless we actually have need for it elsewhere.
  if (auto *union_content = dyn_cast<UnionContent>(&C)) {
    return simplify_union(V, *union_content);
  } else if (auto *cond_content = dyn_cast<ConditionalContent>(&C)) {
    return simplify_conditional(V, *cond_content);
  }

  return C;
}

} // namespace detail

void ContentAnalysisDriver::simplify() {
  // For each content mapping, simplify it.
  for (auto &[value, content] : this->result) {
    // Simplify the content for the given value mapping.
    const auto [domain, range] = content;
    auto &simplified_domain = detail::simplify(*value, *domain);
    auto &simplified_range = detail::simplify(*value, *range);

    // Update the content mapping.
    content.first = &simplified_domain;
    content.second = &simplified_range;
  }

  // Print the initial contents.
  println();
  println("================================");
  println("Simplified contents:");
  for (const auto &[value, contents] : this->result) {
    println("domain(",
            value_name(*value),
            ") contains ",
            contents.first->to_string());
    println("range(",
            value_name(*value),
            ") contains ",
            contents.second->to_string());
  }
  println("================================");
  println();

  // All done.
  return;
}
// =========================================================================

ContentAnalysisDriver::ContentAnalysisDriver(Contents &result, llvm::Module &M)
  : result(result),
    M(M) {

  // Gather the initial contents with a per-instruction analysis.
  this->initialize();

  // Simplify the results to be in a canonical form.
  this->simplify();
}

namespace detail {}

Contents ContentAnalysis::run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
  Contents result;

  // Construct and run content analysis.
  ContentAnalysisDriver(result, M);

  // Return the analyzed contents.
  return result;
}

llvm::AnalysisKey ContentAnalysis::Key;

} // namespace folio
