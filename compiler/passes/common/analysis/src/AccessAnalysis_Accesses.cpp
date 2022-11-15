#include "common/analysis/AccessAnalysis.hpp"

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
        getAccessSummary(I);
      }
    }
  }

  return;
}

AccessSummary *AccessAnalysis::getAccessSummary(llvm::Value &value) {
  /*
   * Check if we have a memoized AccessSummary for this LLVM Value.
   * If we do, return it.
   */
  auto found_summary = this->access_summaries.find(&value);
  if (found_summary != this->access_summaries.end()) {
    return found_summary->second;
  }

  /*
   * Check that this value is a call instruction.
   * If it is, see if it has an AccessSummary.
   * If it isnt, then this value doesn't have an AccessSummary, return nullptr.
   */
  if (auto call_inst = dyn_cast<CallInst>(&value)) {
    return this->getAccessSummaryForCall(*call_inst);
  }

  /*
   * Otherwise, this isn't a MemOIR access, return NULL.
   */
  return nullptr;
}

set<AccessSummary *> &AccessAnalysis::getFieldAccesses(FieldSummary &field) {
  /*
   * Check if we have a memoized set of AccessSummary(s) for this FieldSummary.
   * If we do, return it.
   */
  auto found_accesses = this->field_accesses.find(&field);
  if (found_accesses != this->field_accesses.end()) {
    return found_accesses->second;
  }

  /*
   * Initialize the field accesses for this field summary.
   */
  auto &field_accesses = this->field_accesses[&field];
  field_accesses.clear();

  /*
   * Track all uses of the original allocation to find accesses to this field.
   *  - Track down the object summaries we are using, tracking the chain of
   *    field accesses needed to recreate it.
   */
  stack<FieldSummary *> field_stack;
  field_stack.push(&field);

  auto &object_summary = field.pointsTo();
  while (object_summary.isNested()) {
    auto &nested_struct_summary =
        static_cast<NestedObjectSummary &>(object_summary);
    auto &field_summary = nested_struct_summary.getField();
    field_stack.push(&field_summary);

    object_summary = field_summary.pointsTo();
  }

  /*
   * Return the field accesses for this field summary.
   */
  return field_accesses;
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
   * We don't handle indirect calls at the moment as they should
   *   be statically resolved.
   */
  if (callee == nullptr) {
    return nullptr;
  }

  auto callee_enum = getMemOIREnum(*callee);

  /*
   * If the callee is not a MemOIR access, return NULL.
   */
  if (!isAccess(callee_enum)) {
    return nullptr;
  }

  /*
   * Get the FieldSummary/ies for the given MemOIR access call.
   */
  auto field_arg = call_inst.getArgOperand(0);
  assert(field_arg
         && "in AccessAnalysis::getAccessSummary"
            "field passed into MemOIR access is NULL");

  auto &field_summaries = this->getFieldSummaries(*field_arg);
  assert(!field_summaries.empty()
         && "in AccessAnalysis::getAccessSummary"
            "found no possible field summaries for the given MemOIR access");

  /*
   * If the access is a read, create a ReadSummary for it.
   */
  if (isRead(callee_enum)) {
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
  if (isWrite(callee_enum)) {
    /*
     * Get the value being written
     */
    auto value_written = call_inst.getArgOperand(1);
    assert(value_written
           && "in AccessAnalysis::getAccessSummary"
              "value being written is NULL");

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
