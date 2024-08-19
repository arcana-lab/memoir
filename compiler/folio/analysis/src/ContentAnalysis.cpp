#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/Constants.h"

#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Casting.hpp"

#include "folio/analysis/ContentAnalysis.hpp"

using namespace llvm::memoir;

namespace folio {

namespace detail {

// ContentSummary &summarize_fold_resultant(FoldInst &fold) {

//   // Get the accumulator argument of the fold.
//   auto &accum = fold.getAccumulatorArgument();

//   // Iterate over the def-use chain, constructing a summary of the indices
//   and
//   // elements.
//   for (auto &use : accum.uses()) {

//     // TODO
//   }

//   return *summary;
// }

void summarize(Contents &contents, llvm::Value &value, Content &summary) {

  contents[&value] = &summary;

  return;
}

} // namespace detail

Contents ContentAnalysis::run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {

  Contents result;

  // First, create the initial summaries based solely on the local instruction
  // information.
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        // Ensure that the resultant is a collection.
        auto *type = type_of(I);
        if (not isa_and_nonnull<SequenceType>(type)
            and not isa_and_nonnull<AssocArrayType>(type)) {
          continue;
        }

        if (auto *memoir_inst = into<MemOIRInst>(I)) {

          if (auto *alloc = dyn_cast<AllocInst>(memoir_inst)) {
            auto *empty_content = new EmptyContent();

            detail::summarize(result, I, *empty_content);
          } else if (auto *seq_insert_value =
                         dyn_cast<SeqInsertValueInst>(memoir_inst)) {
            // Create the content for the inserted value.
            auto *new_content =
                new ValueContent(seq_insert_value->getValueInserted());

            // Union with the collection content being inserted into.
            auto *base_content =
                new CollectionContent(seq_insert_value->getBaseCollection());
            auto *union_content = new UnionContent(*new_content, *base_content);

          } else if (auto *assoc_write =
                         dyn_cast<AssocWriteInst>(memoir_inst)) {
            // Create the content for the inserted value.
            // TODO: wrap this in an IndexedContent
            auto *new_content =
                new ValueContent(assoc_write->getValueWritten());

            // Union with the collection content being inserted into.
            auto *base_content =
                new CollectionContent(assoc_write->getObjectOperand());
            auto *union_content = new UnionContent(*new_content, *base_content);
          }
        } else if (auto *phi = dyn_cast<llvm::PHINode>(&I)) {
          // If this PHI is a GAMMA, create a union of two ConditionalContents.
          llvm::BasicBlock *if_block, *else_block;
          auto *branch =
              llvm::GetIfCondition(phi->getParent(), if_block, else_block);

          // If the PHI is not a GAMMA, skip it.
          if (not branch) {
            continue;
          }

          // Create the content for the TRUE branch.
          auto *true_value = phi->getIncomingValueForBlock(if_block);
          Content *true_content = new ValueContent(*true_value);

          // Create the content for the FALSE branch.
          auto *false_value = phi->getIncomingValueForBlock(else_block);
          Content *false_content = new ValueContent(*false_value);

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

          // Construct the ValueContents
          auto *lhs_content = new ValueContent(*lhs);
          auto *rhs_content = new ValueContent(*rhs);

          // Construct the ConditionalContents.
          true_content = new ConditionalContent(*true_content,
                                                pred,
                                                *lhs_content,
                                                *rhs_content);
          false_content = new ConditionalContent(*false_content,
                                                 pred,
                                                 *lhs_content,
                                                 *rhs_content);

          // Construct the UnionContent.
          auto *content = new UnionContent(*true_content, *false_content);

          detail::summarize(result, *phi, *content);
        }
      }
    }
  }

  return result;
}

} // namespace folio
