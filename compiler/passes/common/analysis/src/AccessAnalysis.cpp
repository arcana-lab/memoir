#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

AccessAnalysis::AccessAnalysis(Module &M) : M(M) {
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto call_inst = dyn_cast<llvm::CallInst>(&I);
        if (!call_inst) {
          continue;
        }

        /*
         * Build the AccessSummary for this call instruction
         */
        this->getAccessSummary(*call_inst);
      }
    }
  }
}

AccessSummary *AccessAnalysis::getAccessSummary(llvm::CallInst &call_inst) {
  /*
   * Look up the call instruction to see if we have a memoized AccessSummary.
   */
  auto found_summary = access_summaries.find(&call_inst);
  if (found_summary != access_summaries.end()) {
    return found_summary.second;
  }

  /*
   * If the call instruction is not memoized,
   *   then we need to create its AccessSummary
   */
  auto callee = call_inst.getCalledFunction();

  /*
   * If the callee is an indirect call, then return a nullptr
   * We don't handle indirect calls at the moment as they should
   *   be statically resolved.
   */
  if (callee == nullptr) {
    return nullptr;
  }

  auto callee_name = callee->getName();
  auto callee_enum = getMemOIREnum(callee_name);

  /*
   * Build the AccessSummary for the given MemOIR access call.
   */
  AccessSummary *access_summary;
  switch (callee_enum) {
    case MemOIR_Func::READ_INTEGER:
      break;
    case MemOIR_Func::WRITE_INTEGER:
      break;
    case MemOIR_Func::READ_UINT64:
      break;
    case MemOIR_Func::WRITE_UINT64:
      break;
    case MemOIR_Func::READ_UINT32:
      break;
    case MemOIR_Func::WRITE_UINT32:
      break;
    case MemOIR_Func::READ_UINT16:
      break;
    case MemOIR_Func::WRITE_UINT16:
      break;
    case MemOIR_Func::READ_UINT8:
      break;
    case MemOIR_Func::WRITE_UINT8:
      break;
    case MemOIR_Func::READ_INT64:
      break;
    case MemOIR_Func::WRITE_INT64:
      break;
    case MemOIR_Func::READ_INT32:
      break;
    case MemOIR_Func::WRITE_INT32:
      break;
    case MemOIR_Func::READ_INT16:
      break;
    case MemOIR_Func::WRITE_INT16:
      break;
    case MemOIR_Func::READ_INT8:
      break;
    case MemOIR_Func::WRITE_INT8:
      break;
    case MemOIR_Func::READ_FLOAT:
      break;
    case MemOIR_Func::WRITE_FLOAT:
      break;
    case MemOIR_Func::READ_DOUBLE:
      break;
    case MemOIR_Func::WRITE_DOUBLE:
      break;
    case MemOIR_Func::READ_REFERENCE:
      break;
    case MemOIR_Func::WRITE_REFERENCE:
      break;
    case MemOIR_Func::READ_STRUCT:
      break;
    case MemOIR_Func::READ_TENSOR:
      break;
  }

  return nullptr;
}

/*
 * Singleton
 */
AccessAnalysis &AccessAnalysis::get(Module &M) {
  static AccessAnalysis access_analysis = AccessAnalysis(M);

  return access_analysis;
}
} // namespace llvm::memoir
