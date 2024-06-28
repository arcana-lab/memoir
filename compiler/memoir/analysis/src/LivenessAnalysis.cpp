#include <functional>

#include "llvm/IR/CFG.h"

#include "memoir/analysis/LivenessAnalysis.hpp"

namespace llvm::memoir {

// Result queries.
bool LivenessResult::is_live(llvm::Value &V, MemOIRInst &I, bool after) {
  return this->is_live(V, I.getCallInst(), after);
}

bool LivenessResult::is_live(llvm::Value &V, llvm::Instruction &I, bool after) {
  auto &live_set = this->get_live_values(I, after);

  return live_set.find(&V) != live_set.end();
}

std::set<llvm::Value *> &LivenessResult::get_live_values(MemOIRInst &I,
                                                         bool after) {
  return this->get_live_values(I.getCallInst(), after);
}

std::set<llvm::Value *> &LivenessResult::get_live_values(llvm::Instruction &I,
                                                         bool after) {
  MEMOIR_NULL_CHECK(this->DFR, "Data flow result not available!");

  return after ? this->DFR->OUT(&I) : this->DFR->IN(&I);
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
