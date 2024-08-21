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
  auto *type = type_of(A);

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

Content &ContentAnalysisDriver::visitStructReadInst(StructReadInst &I) {
  // Get the struct content.
  auto &struct_content = this->analyze(I);

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

    // For a fold function, get the corresponding live-out content,
    // and substitute the collection context.

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

    // TODO
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

namespace detail {

Content &contextualize_scalar(llvm::Value &V) {
  // TODO
  return Content::create<ScalarContent>(V);
}

} // namespace detail

void ContentAnalysisDriver::contextualize() {
  // Improve the quality of each existing content based on the use context.
  for (auto &[value, contents] : this->result) {

    if (auto *memoir_inst = into<MemOIRInst>(value)) {
      if (auto *ret_phi = dyn_cast<RetPHIInst>(memoir_inst)) {
      }
    }
  }

  // Print the initial contents.
  println();
  println("================================");
  println("Contextualize contents:");
  for (const auto &[value, contents] : this->result) {
    println(value_name(*value), " contains ", contents->to_string());
  }
  println("================================");
  println();

  return;
}

void ContentAnalysisDriver::simplify() {
  return;
}

ContentAnalysisDriver::ContentAnalysisDriver(Contents &result, llvm::Module &M)
  : result(result),
    M(M) {

  // Gather the initial contents with a per-instruction analysis.
  this->initialize();

  // Contextualize the contents based on their position in the program.
  this->contextualize();

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
