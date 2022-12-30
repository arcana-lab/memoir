#include "memoir/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

void AccessAnalysis::analyzeAccesses() {
  errs() << "AccessAnalysis: analyzing accesses.\n";

  /*
   * Compute the accesses of the program.
   */
  for (auto &F : M) {
    if (MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
      continue;
    }

    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        this->getAccessSummary(I);
      }
    }
  }

  return;
}

AccessSummary *AccessAnalysis::getAccessSummary(llvm::Value &value) {
  /*
   * Check if we have a memoized AccessSummary for this LLVM Value.
   *  - If we do, return it.
   */
  auto found_summary = this->access_summaries.find(&value);
  if (found_summary != this->access_summaries.end()) {
    return found_summary->second;
  }

  /*
   * Check that this value is a call instruction.
   *  - If it is, see if it has an AccessSummary.
   *  - If it isnt, then this value doesn't have an AccessSummary,
   *      return nullptr.
   */
  if (auto call_inst = dyn_cast<CallInst>(&value)) {
    return this->getAccessSummaryForCall(*call_inst);
  }

  /*
   * Otherwise, this isn't a MemOIR access, return NULL.
   */
  return nullptr;
}

AccessSummary *AccessAnalysis::getAccessSummaryForCall(
    llvm::CallInst &call_inst) {
  /*
   * Look up the call instruction to see if we have already created an
   *   AccessSummary for it.
   */
  auto found_summary = this->access_summaries.find(&call_inst);
  if (found_summary != this->access_summaries.end()) {
    return found_summary->second;
  }

  /*
   * If the call instruction is not memoized,
   *   then we need to create its AccessSummary.
   */
  auto callee = call_inst.getCalledFunction();

  /*
   * If the callee is an indirect call, then return a nullptr
   * We don't handle indirect calls at the moment as memoir calls should
   *   be statically resolved.
   */
  if (callee == nullptr) {
    return nullptr;
  }

  auto callee_enum = getMemOIREnum(*callee);

  /*
   * If the callee is not a MemOIR access, return NULL.
   */
  if (!FunctionNames::is_access(callee_enum)) {
    return nullptr;
  }

  /*
   * If the access is a read, create a ReadSummary for it.
   */
  if (FunctionNames::is_read(callee_enum)) {
    /*
     * Determine if this is a write to a struct or a collection.
     */
    auto field_arg = call_inst.getArgOperand(0);
    MEMOIR_NULL_CHECK(field_arg,
                      "collection passed into MemOIR access is NULL");

    auto &collection_summaries = this->getCollectionSummary(*field_arg);
    MEMOIR_ASSERT(
        !field_summaries.empty(),
        "found no possible field summaries for the given MemOIR access");

    /*
     * Create the Read Summary for this access.
     */
    auto read_summary = new ReadSummary(call_inst, field_summaries);
    this->access_summaries[&call_inst] = read_summary;
    for (auto field_summary : field_summaries) {
      this->field_accesses[field_summary].insert(read_summary);
    }

    return read_summary;
  }

  /*
   * If the access is a write, create a WriteSummary for it.
   */
  if (FunctionNames::is_write(callee_enum)) {
    /*
     * Get the FieldSummary/ies for the given MemOIR access call.
     */
    auto field_arg = call_inst.getArgOperand(1);
    MEMOIR_NULL_CHECK(field_arg, "field passed into MemOIR access is NULL");

    auto &field_summaries = this->getCollectionSummary(*field_arg);
    MEMOIR_ASSERT(
        !field_summaries.empty(),
        "found no possible field summaries for the given MemOIR access");

    /*
     * Get the value being written
     */
    auto value_written = call_inst.getArgOperand(0);
    MEMOIR_NULL_CHECK(value_written, "value being written is NULL");

    /*
     * If there are more than one possible fields, return a MAY WriteSummary.
     */
    auto write_summary =
        new WriteSummary(call_inst, field_summaries, *value_written);
    this->access_summaries[&call_inst] = write_summary;
    for (auto field_summary : field_summaries) {
      this->field_accesses[field_summary].insert(write_summary);
    }

    return write_summary;
  }

  /*
   * If we fell through for whatever reason, return NULL.
   */
  return nullptr;
}

} // namespace llvm::memoir