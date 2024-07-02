#include <functional>

#include "llvm/IR/CFG.h"

#include "memoir/analysis/Liveness.hpp"

namespace llvm::memoir {

// Result queries.
bool LivenessResult::is_live(llvm::Value &V, MemOIRInst &I, bool after) {
  return this->is_live(V, I.getCallInst(), after);
}

bool LivenessResult::is_live(llvm::Value &V, llvm::Instruction &I, bool after) {
  auto &live_set = after ? this->DFR->OUT(&I) : this->DFR->IN(&I);

  return live_set.find(&V) != live_set.end();
}

set<llvm::Value *> LivenessResult::live_values(MemOIRInst &I, bool after) {
  return this->live_values(I.getCallInst(), after);
}

set<llvm::Value *> LivenessResult::live_values(llvm::Instruction &I,
                                               bool after) {
  set<llvm::Value *> result;

  auto &live_set = after ? this->DFR->OUT(&I) : this->DFR->IN(&I);

  result.insert(live_set.begin(), live_set.end());

  return result
}

set<llvm::Value *> LivenessResult::live_values(llvm::BasicBlock &From,
                                               llvm::BasicBlock &To) {
  set<llvm::Value *> result;

  // The resulting live set is
  //     (IN[To.FirstNonPHI] \ PhiDefs[To])
  //   U PhiUses[To | incoming=From]
  //   U { v in OUT[From] | v in PhiDefs[To] }
  //
  // This means that, for a variable to be in the live set between From and To:
  //     it must be alive at the beginning of non-PHIs, and not one of the PHIs
  //     defined in this function.
  // OR, it must be used by a PHI, where the incoming block is From.

  // Insert IN[To.FirstNonPHI]
  auto &nonphi_set = this->DFR->IN(To.getFirstNonPHI());
  result.insert(nonphi_set.begin(), nonphi_set.end());

  // Remove PhiDefs[To]
  for (auto *phi = To.phis()) {
    result.remove(phi);
  }

  // Insert PhiUses[To | incoming=From]
  for (auto *phi : To.phis()) {
    auto *incoming = phi->getIncomingValueForBlock(&From);
    result.insert(incoming);
  }

  // Insert { v in OUT[From] | v in PhiDefs[To] }
  auto &from_out_set = this->DFR->OUT(From.getTerminator());
  for (auto *phi : To.phis()) {
    // If phi is in OUT[From], insert it into the result.
    if (from_out_set.count(phi) != 0) {
      result.insert(phi);
    }
  }

  // Return the result.
  return result
}

// Transfer functions.
void compute_gen(llvm::Instruction *inst,
                 arcana::noelle::DataFlowResult *result) {
  // GEN = Use(I)
  for (auto &operand : inst->operands()) {
    auto *operand_value = operand.get();
    if (Type::value_is_collection_type(*operand_value)) {
      result->GEN(inst).insert(operand_value);
    }
  }
}

void compute_kill(llvm::Instruction *inst,
                  arcana::noelle::DataFlowResult *result) {
  // KILL = Def(I)
  if (Type::value_is_collection_type(*inst)) {
    result->KILL(inst).insert(inst);
  }
}

void compute_in(llvm::Instruction *inst,
                std::set<llvm::Value *> &in,
                arcana::noelle::DataFlowResult *result) {
  auto &gen = result->GEN(inst);
  auto &kill = result->KILL(inst);
  auto &out = result->OUT(inst);
  // IN = (OUT-KILL) U GEN
  in.insert(out.begin(), out.end());
  for (auto *kill_value : kill) {
    auto found = in.find(kill_value);
    if (found != in.end()) {
      in.erase(found);
    }
  }
  in.insert(gen.begin(), gen.end());
}

void compute_out(llvm::Instruction *successor,
                 std::set<llvm::Value *> &out,
                 arcana::noelle::DataFlowResult *result) {
  if (isa<llvm::PHINode>(successor)
      && successor == &*successor->getParent()->begin()) {
    auto *successor_bb = successor->getParent();
    auto *first_non_phi = successor_bb->getFirstNonPHI();
    auto &in = result->OUT(first_non_phi);

    // Find which basic block we are coming from.
    // NOTE: this should be fixed in NOELLE by having an option to use a
    //        BasicBlockEdge as successor.
    llvm::BasicBlock *incoming_bb = nullptr;
    for (auto *pred_bb : llvm::predecessors(successor_bb)) {
      auto *pred_term = pred_bb->getTerminator();
      // Check if @out == result->OUT(pred_term)
      if (&out == &result->OUT(pred_term)) {
        incoming_bb = pred_bb;
        break;
      }
    }

    MEMOIR_NULL_CHECK(
        incoming_bb,
        "Couldn't find the out set matching the predecessor. "
        "Blame NOELLE, then go fix its data flow engine with the aforementioned fix.");

    // Compute the IN set for this basic block edge.
    std::set<llvm::Value *> path_sensitive_in(in.begin(), in.end());
    for (auto &phi : successor_bb->phis()) {
      auto *incoming_value = phi.getIncomingValueForBlock(incoming_bb);
      path_sensitive_in.insert(incoming_value);

      auto found_def = path_sensitive_in.find(&phi);
      if (found_def != path_sensitive_in.end()) {
        path_sensitive_in.erase(found_def);
      }
    }

    // OUT = U_succ IN
    out.insert(path_sensitive_in.begin(), path_sensitive_in.end());
  } else {
    auto &in = result->OUT(successor);
    // OUT = U_succ IN
    out.insert(in.begin(), in.end());
  }
}

// Constructor and analysis invocation.
LivenessDriver::LivenessDriver(llvm::Function &F,
                               arcana::noelle::DataFlowEngine DFE)
  : F(F),
    DFE(std::move(DFE)) {
  debugln("Start liveness analysis");

  // this->result.DFR = this->DFE.applyBackward(&F,
  //                                            compute_gen,
  //                                            compute_kill,
  //                                            compute_in,
  //                                            compute_out);
  MEMOIR_UNREACHABLE(
      "LivenessAnalysis needs to be updated to use new NOELLE DFE.");

  debugln("End liveness analysis");
}

} // namespace llvm::memoir
