#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

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
    case MemOIR_Func::WRITE_INTEGER:
    case MemOIR_Func::READ_UINT64:
    case MemOIR_Func::WRITE_UINT64:
    case MemOIR_Func::READ_UINT32:
    case MemOIR_Func::WRITE_UINT32:
    case MemOIR_Func::READ_UINT16:
    case MemOIR_Func::WRITE_UINT16:
    case MemOIR_Func::READ_UINT8:
    case MemOIR_Func::WRITE_UINT8:
    case MemOIR_Func::READ_INT64:
    case MemOIR_Func::WRITE_INT64:
    case MemOIR_Func::READ_INT32:
    case MemOIR_Func::WRITE_INT32:
    case MemOIR_Func::READ_INT16:
    case MemOIR_Func::WRITE_INT16:
    case MemOIR_Func::READ_INT8:
    case MemOIR_Func::WRITE_INT8:
    case MemOIR_Func::READ_FLOAT:
    case MemOIR_Func::WRITE_FLOAT:
    case MemOIR_Func::READ_DOUBLE:
    case MemOIR_Func::WRITE_DOUBLE:
    case MemOIR_Func::READ_REFERENCE:
    case MemOIR_Func::WRITE_REFERENCE:
    case MemOIR_Func::READ_STRUCT:
    case MemOIR_Func::READ_TENSOR:
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

AccessAnalysis::AccessAnalysis(Module &M) : M(M) {
  // Do nothing.
}

} // namespace llvm::memoir
