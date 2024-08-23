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

Content &conservative(llvm::Value &V) {
  auto *type = type_of(V);

  if (isa_and_nonnull<SequenceType>(type)
      or isa_and_nonnull<AssocArrayType>(type)) {
    return Content::create<CollectionContent>(V);
  } else if (isa_and_nonnull<StructType>(type)) {
    return Content::create<StructContent>(V);
  }

  return Content::create<ScalarContent>(V);
}

Content &conservative(MemOIRInst &I) {
  return detail::conservative(I.getCallInst());
}

} // namespace detail

void ContentAnalysisDriver::summarize(llvm::Value &value, Content &summary) {

  this->result[&value] = &summary;

  return;
}

void ContentAnalysisDriver::summarize(MemOIRInst &I, Content &summary) {

  auto &value = I.getCallInst();

  this->result[&value] = &summary;

  return;
}

Content &ContentAnalysisDriver::analyze(llvm::Value &V) {

  // Check if we have already analyzed this value.
  auto found = this->result.find(&V);
  if (found != this->result.end()) {
    return *(found->second);
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
    auto &content = this->visitArgument(*arg);
    this->summarize(V, content);
    return content;
  } else if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    auto &content = this->visit(*inst);
    this->summarize(V, content);
    return content;
  }

  // If the value isn't handled by any of our handlers, output a conservative
  // result.
  return Content::create<ScalarContent>(V);
}

Content &ContentAnalysisDriver::analyze(MemOIRInst &I) {
  return this->analyze(I.getCallInst());
}

Content &ContentAnalysisDriver::visitArgument(llvm::Argument &A) {
  return detail::conservative(A);
}

Content &ContentAnalysisDriver::visitInstruction(llvm::Instruction &I) {
  auto *type = type_of(I);

  if (isa_and_nonnull<SequenceType>(type)
      or isa_and_nonnull<AssocArrayType>(type)) {

    return Content::create<CollectionContent>(I);

  } else if (isa_and_nonnull<StructType>(type)) {

    return Content::create<StructContent>(I);

  } else {

    return Content::create<ScalarContent>(I);
  }
}

Content &ContentAnalysisDriver::visitSequenceAllocInst(SequenceAllocInst &I) {
  auto &empty = Content::create<EmptyContent>();

  return empty;
}

Content &ContentAnalysisDriver::visitAssocArrayAllocInst(
    AssocArrayAllocInst &I) {
  auto &empty = Content::create<EmptyContent>();

  return empty;
}

Content &ContentAnalysisDriver::visitSeqInsertInst(SeqInsertInst &I) {
  // If the element type is a struct, create a TupleContent with empty fields.
  auto *type = type_of(I.getResultCollection());
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(type),
                      "SeqInsertInst on non-collection type");
  auto *struct_type = dyn_cast<StructType>(&collection_type.getElementType());
  if (not struct_type) {
    return detail::conservative(I.getCallInst());
  }

  vector<Content *> fields(struct_type->getNumFields(),
                           &Content::create<EmptyContent>());

  auto &tuple_content = Content::create<TupleContent>(fields);

  auto &base_content = this->analyze(I.getBaseCollection());

  return Content::create<UnionContent>(tuple_content, base_content);
}

Content &ContentAnalysisDriver::visitIndexWriteInst(IndexWriteInst &I) {

  // If the element type is a struct, create a TupleContent with fields.
  auto &collection_type = I.getCollectionType();
  auto &element_type = collection_type.getElementType();
  auto *struct_type = dyn_cast<StructType>(&element_type);
  if (not struct_type) {
    return detail::conservative(I.getCallInst());
  }

  // Create a TupleContent and union it with the input.
  vector<Content *> fields(struct_type->getNumFields(),
                           &Content::create<EmptyContent>());

  // Update the relevant field.
  auto &field_content = this->analyze(I.getValueWritten());

  fields[I.getSubIndex(0)] = &field_content;

  // Create the TupleContent.
  auto &tuple_content = Content::create<TupleContent>(fields);

  // Fetch the content of the input collection.
  auto &base_content = this->analyze(I.getObjectOperand());

  // Return a union of the tuple content and the base content, we will simplify
  // this later.
  // TODO: we may want to add some notion of precedence to the union operator,
  // otherwise we will have overly conservative, or incorrect results from
  // simplification.
  return Content::create<UnionContent>(tuple_content, base_content);
}

Content &ContentAnalysisDriver::visitSeqInsertValueInst(SeqInsertValueInst &I) {

  // Create the content for the inserted value.
  auto &elem_content = this->analyze(I.getValueInserted());

  // Union with the collection content being inserted into.
  auto &base_content = this->analyze(I.getBaseCollection());
  auto &union_content =
      Content::create<UnionContent>(elem_content, base_content);

  return union_content;
}

Content &ContentAnalysisDriver::visitAssocWriteInst(AssocWriteInst &I) {
  // Create the content for the insertion index.
  auto &index_content = this->analyze(I.getKeyOperand());

  // Create the content for the inserted value.
  auto &elem_content = this->analyze(I.getValueWritten());

  // Wrap the index and element in an IndexedContent.
  auto &indexed_content =
      Content::create<IndexedContent>(index_content, elem_content);

  // Union with the collection content being inserted into.
  auto &base_content = this->analyze(I.getObjectOperand());
  auto &union_content =
      Content::create<UnionContent>(indexed_content, base_content);

  return union_content;
}

Content &ContentAnalysisDriver::visitAssocInsertInst(AssocInsertInst &I) {

  auto *struct_type = dyn_cast<StructType>(type_of(I));
  if (not struct_type) {
    return detail::conservative(I);
  }

  // If the element type is a struct, create a TupleContent with empty fields.
  vector<Content *> fields(struct_type->getNumFields(),
                           &Content::create<EmptyContent>());
  auto &tuple_content = Content::create<TupleContent>(fields);

  // Create the content for the insertion index.
  auto &index_content = this->analyze(I.getInsertionPoint());

  // Wrap the index and element in an IndexedContent.
  auto &indexed_content =
      Content::create<IndexedContent>(index_content, tuple_content);

  // Union with the collection content being inserted into.
  auto &base_content = this->analyze(I.getBaseCollection());
  auto &union_content =
      Content::create<UnionContent>(indexed_content, base_content);

  return union_content;
}

Content &ContentAnalysisDriver::visitStructReadInst(StructReadInst &I) {
  // Get the struct content.
  auto &struct_content = this->analyze(I.getObjectOperand());

  // Get the field index;
  auto field_index = I.getFieldIndex();

  // Create the FieldContent.
  auto &field_content =
      Content::create<FieldContent>(struct_content, field_index);

  return field_content;
}

Content &ContentAnalysisDriver::visitFoldInst(FoldInst &I) {
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
  auto &returned_content = this->analyze(*returned);

  // Fetch the content of the initial value.
  auto &initial_content = this->analyze(I.getInitial());

  // Fetch the content of the input collection.
  auto &collection_content = this->analyze(I.getCollection());

  // Substitute the function arguments with the operands.
  auto *accum_content =
      &returned_content
           .substitute(I.getAccumulatorArgument(),
                       this->analyze(I.getInitial()))
           .substitute(I.getIndexArgument(),
                       Content::create<KeysContent>(collection_content));

  // If the element is non-void, fetch its content.
  if (auto *elem_arg = I.getElementArgument()) {
    accum_content = &accum_content->substitute(
        *elem_arg,
        Content::create<ElementsContent>(collection_content));
  }

  // Construct the union'd content of the initial and accumulated.
  return Content::create<UnionContent>(initial_content, *accum_content);
}

Content &ContentAnalysisDriver::visitUsePHIInst(UsePHIInst &I) {
  // Propagate the used collection.
  return this->analyze(I.getUsedCollection());
}

Content &ContentAnalysisDriver::visitPHINode(llvm::PHINode &I) {
  // If this PHI is a copy, propagate the single input.
  if (I.getNumIncomingValues() == 1) {
    auto *incoming_value = I.getIncomingValue(0);
    return this->analyze(*incoming_value);
  }

  // If this PHI is a GAMMA, create a union of two ConditionalContents.
  llvm::BasicBlock *if_block, *else_block;
  auto *branch = llvm::GetIfCondition(I.getParent(), if_block, else_block);

  // If the PHI is not a GAMMA, return a conservative content.
  if (not branch) {
    return detail::conservative(I);
  }

  // Create the content for the TRUE branch.
  auto *true_value = I.getIncomingValueForBlock(if_block);
  auto &true_content = this->analyze(*true_value);

  // Create the content for the FALSE branch.
  auto *false_value = I.getIncomingValueForBlock(else_block);
  auto &false_content = this->analyze(*false_value);

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
  auto &lhs_content = this->analyze(*lhs);
  auto &rhs_content = this->analyze(*rhs);

  // Construct the ConditionalContents.
  auto &if_content = Content::create<ConditionalContent>(true_content,
                                                         pred,
                                                         lhs_content,
                                                         rhs_content);
  auto &else_content = Content::create<ConditionalContent>(
      false_content,
      llvm::CmpInst::getInversePredicate(pred),
      lhs_content,
      rhs_content);

  // Construct the UnionContent.
  return Content::create<UnionContent>(if_content, else_content);
}

Content &ContentAnalysisDriver::visitRetPHIInst(RetPHIInst &I) {

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
      auto metadata = Metadata::get<LiveOutMetadata>(inst);
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
    auto &live_out_content = this->analyze(*live_out);

    // Fetch the content of the initial value.
    auto &input_content = this->analyze(I.getInputCollection());

    // Fetch the content of the collection being folded over.
    auto &collection_content = this->analyze(fold->getCollection());

    // Substitute the function arguments with the operands.
    auto *content =
        &live_out_content.substitute(closed_argument, input_content)
             .substitute(fold->getIndexArgument(),
                         Content::create<KeysContent>(collection_content));

    // If the element is non-void, fetch its content.
    if (auto *elem_arg = fold->getElementArgument()) {
      content = &content->substitute(
          *elem_arg,
          Content::create<ElementsContent>(collection_content));
    }

    // Construct the union'd content of the initial and accumulated.
    return Content::create<UnionContent>(input_content, *content);
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
      auto metadata = Metadata::get<LiveOutMetadata>(inst);
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
    auto &live_out_content = this->analyze(*live_out);

    // Substitute the argument for the operand.
    auto &operand = MEMOIR_SANITIZE(call->getArgOperand(arg_number),
                                    "Argument operand for CallBase is NULL");
    auto &operand_content = this->analyze(operand);
    auto &argument = MEMOIR_SANITIZE(called_function->getArg(arg_number),
                                     "Argument is NULL");
    auto &subst_content =
        live_out_content.substitute(argument, operand_content);
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
    println(value_name(*value), " contains ", contents->to_string());
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
    auto &simplified = detail::simplify(*value, *content);

    // Update the content mapping.
    content = &simplified;
  }

  // Print the initial contents.
  println();
  println("================================");
  println("Simplified contents:");
  for (const auto &[value, contents] : this->result) {
    // Only print the result of fold instructions.
    if (not into<FoldInst>(value)) {
      continue;
    }

    println(value_name(*value), " contains ", contents->to_string());
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

Contents ContentAnalysis::run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {

  Contents result;

  ContentAnalysisDriver(result, M);

  return result;
}

llvm::AnalysisKey ContentAnalysis::Key;

} // namespace folio
