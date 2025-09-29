// LLVM
#include "llvm/IR/CFG.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// MEMOIR
#include "memoir/ir/Builder.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/transform/utilities/Inlining.hpp"

#include "memoir/lower/LowerFold.hpp"

namespace memoir {

struct HeaderInfo {
  HeaderInfo(llvm::BasicBlock &preheader,
             llvm::BasicBlock &header,
             llvm::BasicBlock &body,
             llvm::BasicBlock &exit,
             llvm::PHINode &accumulator,
             llvm::Value &key,
             llvm::Value *element)
    : preheader(preheader),
      header(header),
      body(body),
      exit(exit),
      accumulator(accumulator),
      key(key),
      element(element) {}

  llvm::BasicBlock &preheader;
  llvm::BasicBlock &header;
  llvm::BasicBlock &body;
  llvm::BasicBlock &exit;
  llvm::PHINode &accumulator;
  llvm::Value &key;
  llvm::Value *element;
};

static HeaderInfo lower_fold_header(FoldInst &I,
                                    MemOIRBuilder &builder,
                                    llvm::Value &collection,
                                    CollectionType &collection_type,
                                    llvm::Function &begin_func,
                                    llvm::Function &next_func,
                                    llvm::Type &iter_type) {

  // Split the block before the fold instruction, creating a while loop.
  //   it = alloca
  //   elem = alloca
  //   begin(it, collection)
  //   while (next(it, elem))
  //     {body}
  //   {continuation}
  auto &inst = I.getCallInst();
  auto *preheader =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.preheader.",
                       /* Before = */ false);
  auto *body =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.body.",
                       /* Before = */ false);
  auto *header = llvm::BasicBlock::Create(builder.getContext(),
                                          "fold.loop.header.",
                                          body->getParent(),
                                          body);
  auto *exit =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.exit.",
                       /* Before = */ false);

  // Rewire the branch target of the preheader to the header.
  auto *preheader_branch =
      dyn_cast<llvm::BranchInst>(preheader->getTerminator());
  preheader_branch->setSuccessor(0, header);

  // Construct the contents of the preheader.
  builder.SetInsertPoint(preheader->getFirstInsertionPt());

  // Insert the iterator and element allocations.
  auto *iter_alloca = builder.CreateAlloca(&iter_type);

  // Insert the call to begin.
  auto begin_callee = llvm::FunctionCallee(&begin_func);
  auto *iterator = builder.CreateCall(
      begin_callee,
      llvm::ArrayRef<llvm::Value *>({ iter_alloca, &collection }));

  // Construct the contents of the header.
  builder.SetInsertPoint(header);

  // Create a PHI node for the accumulator value.
  auto *accumulator_llvm_type = I.getCallInst().getType();
  auto &accumulator = MEMOIR_SANITIZE(
      builder.CreatePHI(accumulator_llvm_type, 2, "fold.accum.phi."),
      "Unable to create PHI node for accumulator!");

  // Insert the call to next.
  auto next_callee = llvm::FunctionCallee(&next_func);
  auto *has_next =
      builder.CreateCall(next_callee,
                         llvm::ArrayRef<llvm::Value *>({ iter_alloca }));

  // Replace the current terminator with a conditional branch based on the
  // result of next.
  builder.CreateCondBr(has_next, body, exit);

  // Update the builder's insertion point to the body of the loop.
  builder.SetInsertPoint(body->getFirstInsertionPt());

  // Rewire the branch target of the body's terminator to be the header.
  auto *body_branch = dyn_cast<llvm::BranchInst>(body->getTerminator());
  body_branch->setSuccessor(0, header);

  // Unpack the lowered element type.
  auto &iter_struct_type =
      MEMOIR_SANITIZE(dyn_cast<llvm::StructType>(&iter_type),
                      "Lowered iter type is not a struct!");
  auto *key_type = iter_struct_type.getElementType(0);

  // Load the key.
  auto *key_gep =
      builder.CreateStructGEP(&iter_type, iter_alloca, 0, "fold.key.ptr.");
  auto *key = builder.CreateLoad(key_type, key_gep, "fold.key.");

  // Load the value, if it exists.
  llvm::Value *element = nullptr;
  auto &element_type = collection_type.getElementType();
  if (not isa<VoidType>(&element_type)) {
    auto *element_gep =
        builder.CreateStructGEP(&iter_type, iter_alloca, 1, "fold.elem.ptr.");

    auto *llvm_element_type = iter_struct_type.getElementType(1);
    MEMOIR_NULL_CHECK(llvm_element_type,
                      "LLVM struct type has no type for second field!");
    element = builder.CreateLoad(llvm_element_type, element_gep, "fold.elem.");

    // If the resultant is a boolean, we'll need to cast it.
    if (auto *integer_type = dyn_cast<IntegerType>(&element_type)) {
      if (integer_type->getBitWidth() == 1) {
        element = builder.CreateTrunc(element, builder.getInt1Ty());
      }
    }
  }

  return HeaderInfo(*preheader,
                    *header,
                    *body,
                    *exit,
                    accumulator,
                    *key,
                    element);
}

bool lower_fold(FoldInst &I,
                llvm::Value &collection,
                Type &type,
                llvm::Function *begin_func,
                llvm::Function *next_func,
                llvm::Type *iter_type,
                std::function<void(llvm::Value &, llvm::Value &)> coalesce,
                std::function<void(llvm::Instruction &)> cleanup) {

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&type),
                                          "Lowering fold over non-collection!");

  // Above the fold instruction, create a SizeInst for the collection,
  MemOIRBuilder builder(I);

  // Insert the loop header.
  auto header_info = lower_fold_header(I,
                                       builder,
                                       collection,
                                       collection_type,
                                       *begin_func,
                                       *next_func,
                                       *iter_type);

  // Unpack the header info.
  auto &loop_preheader = header_info.preheader;
  auto &loop_header = header_info.header;
  auto &loop_body = header_info.body;
  auto &loop_exit = header_info.exit;
  auto &accumulator = header_info.accumulator;
  auto &key = header_info.key;
  auto *element = header_info.element;

  // Fetch information about the fold body
  auto &body = I.getBody();
  auto *body_type = body.getFunctionType();

  // Construct the list of arguments to pass into the body.
  Vector<llvm::Value *> arguments = { &accumulator, &key };
  if (element) {
    arguments.push_back(element);
  }
  arguments.insert(arguments.end(), I.closed_begin(), I.closed_end());

  // Create a call to the fold body so that we can inline it.
  auto &call = MEMOIR_SANITIZE(
      builder.CreateCall(body_type, &body, arguments, "fold.accum."),
      "Failed to create call to the FoldInst body!");

  // For each predecessor of the loop header:
  //  - If it _is_ the the loop pre-header, set the incoming value of the
  //    accumulator to be the initial operand of the FoldInst.
  //  - If it is _not_ the loop pre-header, set the incoming value of the
  //    accumulator to be the result of the call.
  for (auto *pred : llvm::predecessors(&loop_header)) {
    auto *incoming = (pred == &loop_preheader) ? &I.getInitial() : &call;
    MEMOIR_ASSERT(accumulator.getType() == incoming->getType(),
                  "Mismatched PHI type for accumulator of ",
                  I);
    accumulator.addIncoming(incoming, pred);
  }

  // Replace all uses of the fold instruction result with the call result.
  coalesce(I.getCallInst(), accumulator);

  // For each closed argument:
  auto closed_idx = -1;
  for (auto *closed : I.closed()) {
    ++closed_idx;

    // Check that it is a collection.
    if (not Type::value_is_object(*closed)) {
      continue;
    }

    // See if it has a RetPHI for the original fold.
    RetPHIInst *found_ret_phi = nullptr;
    for (auto &closed_use : closed->uses()) {
      auto *closed_user = closed_use.getUser();

      if (auto *closed_user_as_inst =
              dyn_cast_or_null<llvm::Instruction>(closed_user)) {

        // If the user is a RetPHI:
        if (auto *ret_phi = into<RetPHIInst>(closed_user_as_inst)) {

          // Ensure that the RetPHI is for FoldInst.
          if (ret_phi->getCalledFunction()
              == I.getCallInst().getCalledFunction()) {

            if (not found_ret_phi) {
              found_ret_phi = ret_phi;
            } else {
              auto *fold_bb = I.getParent();
              auto *new_bb = ret_phi->getParent();
              auto *old_bb = found_ret_phi->getParent();
              if (fold_bb != old_bb and fold_bb == new_bb) {
                found_ret_phi = ret_phi;
              } else if (fold_bb != new_bb and fold_bb == old_bb) {
                continue;
              } else {
                println(*loop_header.getParent());
                println(*ret_phi);
                println(*found_ret_phi);
                MEMOIR_UNREACHABLE("Cannot disambiguate between two RetPHIs!"
                                   "Need to add a dominance check here.");
              }
            }
          } else {
            debugln("wrong function!");
          }
        }
      }
    }

    // If we could not find a RetPHI for the closed argument, then continue.
    if (not found_ret_phi) {
      warnln("Failed to find ret phi for closed operand");
      continue;
    }

    // If it does, we need to patch its DEF-USE chain.
    auto *closed_result = &found_ret_phi->getInput();

    //  -- Replace uses of the original RetPHI with the continuation PHI.
    auto &ret_phi_inst = found_ret_phi->getCallInst();
    coalesce(ret_phi_inst, *closed_result);

    //  -- Delete the old RetPHI.
    cleanup(ret_phi_inst);
  }

  return true;

  // Now, try to inline the fold function.
  llvm::InlineFunctionInfo IFI;
  auto inline_result = memoir::InlineFunction(call, IFI);

  // If inlining failed, send a warning and continue.
  if (not inline_result.isSuccess()) {
    warnln("Inlining fold function failed.\n  Reason: ",
           inline_result.getFailureReason());
  }

  return true;
}

} // namespace memoir
