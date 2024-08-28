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

ContentSummary conservative(llvm::Value &V) {
  auto *type = type_of(V);

  if (isa_and_nonnull<SequenceType>(type)
      or isa_and_nonnull<AssocArrayType>(type)) {
    return { &Content::create<KeysContent>(V),
             &Content::create<ElementsContent>(V) };
  } else if (isa_and_nonnull<StructType>(type)) {
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

ContentSummary ContentAnalysisDriver::analyze(llvm::Value &V) {

  // Check if we have already analyzed this value.
  auto found = this->result.find(&V);
  if (found != this->result.end()) {
    return found->second;
  }

  // Check if we have already visited this value.
  if (visited.count(&V) > 0) {
    // Already visited, return the conservative result.
    return detail::conservative(V);
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
  return detail::conservative(A);
}

ContentSummary ContentAnalysisDriver::visitInstruction(llvm::Instruction &I) {
  return detail::conservative(I);
}

ContentSummary ContentAnalysisDriver::visitSequenceAllocInst(
    SequenceAllocInst &I) {
  auto &empty = Content::create<EmptyContent>();

  return { &Content::create<KeysContent>(I.getCallInst()), &empty };
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
  auto &collection = I.getObjectOperand();
  auto [domain, base_range] = this->recurse ? this->analyze(collection)
                                            : detail::conservative(collection);

  // Analyze the index being written.
  auto [_index_domain, index_range] = this->analyze(I.getIndex());

  // Analyze the value being written.
  auto [_value_domain, value_range] = this->analyze(I.getValueWritten());

  // Get the type information.
  auto &collection_type = I.getCollectionType();
  auto *element_type = &collection_type.getElementType();

  // Handle subindices.
  auto *element_content = value_range;
  for (unsigned sub_dim = 0; sub_dim < I.getNumberOfSubIndices(); ++sub_dim) {

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

  return { domain, &range };
}

ContentSummary ContentAnalysisDriver::visitSeqInsertValueInst(
    SeqInsertValueInst &I) {

  // Fetch the base collection info.
  auto &base = I.getBaseCollection();
  auto [base_domain, base_range] =
      this->recurse ? this->analyze(base) : detail::conservative(base);

  // Fetch the insertion point info.
  auto [_index_domain, index_range] = this->analyze(I.getInsertionPoint());

  // Fetch the inserted value's range.
  auto [_value_domain, value_range] = this->analyze(I.getValueInserted());

  // Construct the new domain.
  auto &domain = Content::create<UnionContent>(*index_range, *base_range);

  // Construct the new range.
  auto &range = Content::create<UnionContent>(*value_range, *base_domain);

  return { &domain, &range };
}

ContentSummary ContentAnalysisDriver::visitAssocWriteInst(AssocWriteInst &I) {

  // Fetch the base collection info.
  // auto [base_domain, base_range] = this->analyze(I.getObjectOperand());
  auto &base = I.getObjectOperand();
  auto [base_domain, base_range] =
      this->recurse ? this->analyze(base) : detail::conservative(base);

  // Fetch the inserted value's range.
  auto [_, value_range] = this->analyze(I.getValueWritten());

  // Construct the new range.
  auto &range = Content::create<UnionContent>(*value_range, *base_range);

  return { base_domain, &range };
}

ContentSummary ContentAnalysisDriver::visitAssocInsertInst(AssocInsertInst &I) {

  // Fetch the base collection info.
  // auto [base_domain, base_range] = this->analyze(I.getBaseCollection());
  auto &base = I.getBaseCollection();
  auto [base_domain, base_range] =
      this->recurse ? this->analyze(base) : detail::conservative(base);

  // Fetch the insertion point.
  auto [_, index_range] = this->analyze(I.getInsertionPoint());

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
  auto [_, elements] = this->recurse
                           ? this->analyze(I.getObjectOperand())
                           : detail::conservative(I.getObjectOperand());
  return detail::handle_collection_read(I, *elements);
}

ContentSummary ContentAnalysisDriver::visitAssocReadInst(AssocReadInst &I) {
  // Construct the resultant content.
  auto [_, elements] = this->recurse
                           ? this->analyze(I.getObjectOperand())
                           : detail::conservative(I.getObjectOperand());
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
  // Construct the KeyContent
  auto [domain, _] = this->recurse ? this->analyze(I.getCollection())
                                   : detail::conservative(I.getCollection());
  return { &Content::create<KeysContent>(I.getCallInst()), domain };
}

namespace detail {
ContentSummary contextualize_fold(FoldInst &I, ContentSummary input) {

  // Fetch the content of the input collection.
  auto [folded_domain, folded_range] = detail::conservative(I.getCollection());

  // Fetch the content of the initial value.
  auto [init_domain, init_range] = detail::conservative(I.getInitial());

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
    auto &closed_arg = I.getClosedArgument(closed_use);

    auto [closed_arg_domain, closed_arg_range] =
        detail::conservative(closed_arg);

    auto [closed_domain, closed_range] = detail::conservative(closed);

    domain = &domain->substitute(*closed_arg_domain, *closed_domain)
                  .substitute(*closed_arg_range, *closed_range);
    range = &range->substitute(*closed_arg_domain, *closed_domain)
                 .substitute(*closed_arg_range, *closed_range);
  }

  return { domain, range };
}

} // namespace detail

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
  auto was_recurse = this->recurse;
  this->recurse = true;
  auto returned_summary = this->analyze(*returned);
  this->recurse = was_recurse;

  // Get the conservative contents of the returned value.
  auto [returned_domain, returned_range] = detail::conservative(*returned);

  // Contextualize the returned value.
  auto [domain, range] = detail::contextualize_fold(I, returned_summary);

  // Substitute the returned value for empty.
  auto &empty = Content::create<EmptyContent>();
  return { &domain->substitute(*returned_domain, empty),
           &range->substitute(*returned_range, empty) };
}

ContentSummary ContentAnalysisDriver::visitUsePHIInst(UsePHIInst &I) {
  // Propagate the used collection.
  return this->analyze(I.getUsedCollection());
}

ContentSummary ContentAnalysisDriver::visitPHINode(llvm::PHINode &I) {

  // If the PHI is a scalar value, just return a ScalarContent.
  auto *type = type_of(I);
  if (not isa_and_nonnull<CollectionType>(type)) {
    // return { &Content::create<UnderdefinedContent>(),
    // &detail::conservative(I) };
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
  set<llvm::Value *> incoming_values = {};
  vector<ContentSummary> incoming_contents = {};
  incoming_contents.reserve(I.getNumIncomingValues());

  for (unsigned i = 0; i < I.getNumIncomingValues(); ++i) {
    auto *incoming_value = I.getIncomingValue(i);
    if (incoming_values.count(incoming_value) > 0) {
      continue;
    }

    incoming_values.insert(incoming_value);
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

    // Analyze the live out
    auto was_recurse = this->recurse;
    this->recurse = true;
    auto live_out_summary = this->analyze(*live_out);
    this->recurse = was_recurse;

    // Otherwise, get the content of the live out.
    auto [domain, range] =
        detail::contextualize_fold(*fold, this->analyze(*live_out));

    // Substitute the conservative live out with empty.
    auto [live_out_domain, live_out_range] = detail::conservative(*live_out);
    auto &empty = Content::create<EmptyContent>();
    return { &domain->substitute(*live_out_domain, empty),
             &range->substitute(*live_out_range, empty) };
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
    auto [arg_domain, arg_range] = detail::conservative(argument);

    auto &subst_domain =
        live_out_domain->substitute(*arg_domain, *operand_domain);
    auto &subst_range = live_out_range->substitute(*arg_range, *operand_range);

    return { &subst_domain, &subst_range };
  }

  // For an indirect call, we will conservatively say that we know
  // nothing.
  return detail::conservative(I.getCallInst());
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
Content *ContentAnalysisDriver::lookup_domain(llvm::Value &V) {
  // Lookup the content.
  auto found = this->result.find(&V);
  if (found == this->result.end()) {
    return nullptr;
  }

  auto [domain, _] = found->second;

  // The result must be in a simplified form.
  if (isa<TupleContent>(domain) or isa<EmptyContent>(domain)) {
    return domain;
  }

  return nullptr;
}

Content *ContentAnalysisDriver::lookup_range(llvm::Value &V) {
  // Lookup the content.
  auto found = this->result.find(&V);
  if (found == this->result.end()) {
    return nullptr;
  }

  auto [_, range] = found->second;

  // The result must be in a simplified form.
  if (isa<TupleContent>(range) or isa<EmptyContent>(range)) {
    return range;
  }

  return nullptr;
}

Content &ContentAnalysisDriver::simplifyConditionalContent(
    ConditionalContent &C,
    llvm::Value *V) {
  // Recurse.
  auto &content = this->simplify(C.content(), V);

  // empty | c == empty
  if (isa<EmptyContent>(&content)) {
    return content;
  }

  // If recursion simplified the inner content, update ourselves.
  if (&content != &C.content()) {
    return Content::create<ConditionalContent>(content,
                                               C.predicate(),
                                               C.lhs(),
                                               C.rhs());
  }

  return C;
}

Content &ContentAnalysisDriver::simplifyKeysContent(KeysContent &C,
                                                    llvm::Value *V) {

  // V :- Keys(V) == empty
  // if (V and &C.collection() == V) {
  //   return Content::create<EmptyContent>();
  // }

  if (auto *lookup = this->lookup_domain(C.collection())) {
    return *lookup;
  }

  return C;
}

Content &ContentAnalysisDriver::simplifyElementsContent(ElementsContent &C,
                                                        llvm::Value *V) {

  // V :- Elements(V) == empty
  // if (V and &C.collection() == V) {
  //   return Content::create<EmptyContent>();
  // }

  if (auto *lookup = this->lookup_range(C.collection())) {
    return *lookup;
  }

  return C;
}

Content &ContentAnalysisDriver::simplifyUnionContent(UnionContent &C,
                                                     llvm::Value *V) {

  // Unpack the union.
  auto &lhs = this->simplify(C.lhs(), V);
  auto &rhs = this->simplify(C.rhs(), V);

  // C U empty = C
  if (isa<EmptyContent>(&rhs)) {
    return lhs;
  }

  // empty U C = C
  if (isa<EmptyContent>(&lhs)) {
    return rhs;
  }

  // C U C = C
  if (lhs == rhs) {
    return lhs;
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
      auto &lhs_elem = this->simplify(lhs_tuple->element(i), V);
      auto &rhs_elem = this->simplify(rhs_tuple->element(i), V);

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

  // If nothing happened, then see if we can reassociate the union and go
  // again.
  if (auto *rhs_union = dyn_cast<UnionContent>(&rhs)) {
    auto &rhs_lhs = rhs_union->lhs();
    auto &rhs_rhs = rhs_union->rhs();
    auto &new_union =
        this->simplify(Content::create<UnionContent>(lhs, rhs_lhs), V);

    return this->simplify(Content::create<UnionContent>(new_union, rhs_rhs), V);
  }

  // If we weren't able to reassociate, create a new union content if any
  // sub-simplifications happened.
  if (&lhs != &C.lhs() or &rhs != &C.rhs()) {
    return this->simplify(Content::create<UnionContent>(lhs, rhs), V);
  }

  return C;
}

Content &ContentAnalysisDriver::simplify(Content &C, llvm::Value *V) {
  // Dispatch to the appropriate content.
  // NOTE: this could be replaced with a ContentVisitor, but I don't want to
  // implement one unless we actually have need for it elsewhere.
  if (auto *union_content = dyn_cast<UnionContent>(&C)) {
    return this->simplifyUnionContent(*union_content, V);
  } else if (auto *cond_content = dyn_cast<ConditionalContent>(&C)) {
    return this->simplifyConditionalContent(*cond_content, V);
  } else if (auto *elements_content = dyn_cast<ElementsContent>(&C)) {
    return this->simplifyElementsContent(*elements_content, V);
  } else if (auto *keys_content = dyn_cast<KeysContent>(&C)) {
    return this->simplifyKeysContent(*keys_content, V);
  }

  return C;
}

void ContentAnalysisDriver::simplify() {

  int timeout = 10;
  for (int i = 0; i < timeout; ++i) {

    // For each content mapping, simplify it.
    for (auto &[value, content] : this->result) {

      // Simplify the content for the given value mapping.
      const auto [domain, range] = content;

      auto &simplified_domain = this->simplify(
          domain->substitute(Content::create<KeysContent>(*value),
                             Content::create<EmptyContent>()),
          value);

      auto &simplified_range = this->simplify(
          range->substitute(Content::create<ElementsContent>(*value),
                            Content::create<EmptyContent>()),
          value);

      // Update the content mapping.
      content.first = &simplified_domain;
      content.second = &simplified_range;
    }
  }

  // Print the initial contents.
  println();
  println("================================");
  println("Simplified contents:");
  for (const auto &[value, contents] : this->result) {
    if (not into<RetPHIInst>(value) and not into<FoldInst>(value)) {
      continue;
    }

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
    M(M),
    recurse(false) {

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
