#include "memoir/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

void AccessAnalysis::analyzeStructs() {
  errs() << "AccessAnalysis: analyzing objects.\n";

  /*
   * Compute the objects for the program.
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
        getStructSummary(I);
      }
    }
  }

  return;
}

StructSummary *AccessAnalysis::getStructSummary(llvm::Value &value) {
  /*
   * See if we have a memoized set of StructSummaries for this LLVM Value.
   *  - If we do, return it.
   *  - Otherwise, we will build them.
   */
  auto found_summaries = this->value_to_struct_summaries.find(&value);
  if (found_summaries != this->value_to_struct_summaries.end()) {
    return found_summaries->second;
  }

  /*
   * Initialize the object to summaries for this value.
   */
  auto &struct_summaries = this->value_to_struct_summaries[&value];
  struct_summaries.clear();

  /*
   * Check if the value is a call to readStruct, readTensor or readReference
   *  - If it is, fetch the object summaries for it.
   */
  if (auto call_inst = dyn_cast<CallInst>(&value)) {
    auto callee_enum = getMemOIREnum(*call_inst);

    switch (callee_enum) {
      case MemOIR_Func::GET_OBJECT: {
        return this->getReadStructSummaries(*call_inst);
      }
      case MemOIR_Func::READ_REFERENCE: {
        return this->getReadReferenceSummaries(*call_inst);
      }
      default: {
        break;
      }
    }
  }

  /*
   * Check if the value is a lossless cast.
   *  - If it is, recurse on its value
   */
  if (auto cast_inst = dyn_cast<CastInst>(&value)) {
    /*
     * If this is a lossy cast, return the empty set.
     */
    if (!cast_inst->isLosslessCast()) {
      struct_summaries.clear();
      return struct_summaries;
    }

    /*
     * Otherwise, recurse on the operand to the cast instruction.
     */
    auto operand_value = cast_inst->getOperand(0);
    assert(operand_value
           && "in AccessAnalysis::getObjectSummaries"
              "operand for CastInst is NULL");

    auto &operand_struct_summaries = this->getObjectSummaries(*operand_value);
    struct_summaries.insert(operand_struct_summaries.begin(),
                            operand_struct_summaries.end());

    return struct_summaries;
  }

  /*
   * Otherwise, fetch the AllocationSummaries and wrap them in
   *   BaseObjectSummaries.
   */
  auto &allocation_analysis = AllocationAnalysis::get(M);
  auto &allocation_summaries =
      allocation_analysis.getAllocationSummaries(value);

  /*
   * Wrap each of the bare AllocationSummaries with a BaseObjectSummary
   */
  for (auto allocation_summary : allocation_summaries) {
    /*
     * See if we have a memoized ObjectSummary for this AllocationSummary.
     *  - If we do, use it.
     *  - Otherwise, build it and memoize it.
     */
    BaseObjectSummary *base_struct_summary;
    auto found_summary = this->base_struct_summaries.find(allocation_summary);
    if (found_summary != this->base_struct_summaries.end()) {
      base_struct_summary = found_summary->second;
    } else {
      base_struct_summary = new BaseObjectSummary(*allocation_summary);

      this->base_struct_summaries[allocation_summary] = base_struct_summary;
    }

    struct_summaries.insert(base_struct_summary);
  }

  /*
   * Return a set of the ObjectSummaries
   */
  return struct_summaries;
}

set<ObjectSummary *> &AccessAnalysis::getReadStructSummaries(
    llvm::CallInst &call_inst) {
  /*
   * Determine the FieldSummaries this readStruct could be accessing.
   */
  auto field_arg = call_inst.getArgOperand(0);
  auto &field_summaries = this->getFieldSummaries(*field_arg);

  /*
   * Build NestedStructSummaries for the given FieldSummaries.
   */
  auto &value_struct_summaries = this->value_to_struct_summaries[&call_inst];
  for (auto field_summary : field_summaries) {
    /*
     * See if we have a memoized set of ObjectSummaries for this FieldSummary.
     * If we do, add it to the set.
     */
    auto found_summary = this->nested_struct_summaries.find(field_summary);
    if (found_summary != this->nested_struct_summaries.end()) {
      auto struct_summary = found_summary->second;
      value_struct_summaries.insert(struct_summary);
      continue;
    }

    /*
     * Create the new NestedObjectSummary.
     */
    auto struct_summary = new NestedObjectSummary(*field_summary);

    /*
     * Store the owned NestedObjectSummary pointer.
     */
    nested_struct_summaries[field_summary] = struct_summary;

    /*
     * Store the shared NestedObjectSummary pointer.
     */
    value_struct_summaries.insert(struct_summary);
  }

  /*
   * Return the set of object summaries.
   */
  return value_struct_summaries;
}

set<ObjectSummary *> &AccessAnalysis::getReadTensorSummaries(
    llvm::CallInst &call_inst) {
  /*
   * Determine the FieldSummaries this readStruct could be accessing.
   */
  auto field_arg = call_inst.getArgOperand(0);
  auto &field_summaries = this->getFieldSummaries(*field_arg);

  /*
   * Build NestedTensorSummaries for the given FieldSummaries.
   */
  auto &struct_summaries = this->value_to_struct_summaries[&call_inst];
  for (auto field_summary : field_summaries) {
    /*
     * See if we have a memoized set of ObjectSummaries for this CallInst.
     * If we do, insert it into the set.
     */
    auto found_summary = this->nested_struct_summaries.find(field_summary);
    if (found_summary != this->nested_struct_summaries.end()) {
      auto struct_summary = found_summary->second;
      struct_summaries.insert(struct_summary);
      continue;
    }

    /*
     * Otherwise, create the new NestedTensorSummary.
     */
    auto struct_summary = new NestedObjectSummary(*field_summary);

    /*
     * Store the owned NestedObjectSummary pointer.
     */
    nested_struct_summaries[field_summary] = struct_summary;

    /*
     * Store the shared NestedObjectSummary pointer.
     */
    struct_summaries.insert(struct_summary);
  }

  /*
   * Return the object summaries.
   */
  return struct_summaries;
}

set<ObjectSummary *> &AccessAnalysis::getReadReferenceSummaries(
    llvm::CallInst &call_inst) {
  /*
   * See if we have a memoized set of ObjectSummaries for this CallInst.
   *  - If we do, return it.
   *  - Otherwise, create the NestedStructSummaries.
   */
  auto found_summaries = this->value_to_struct_summaries.find(&call_inst);
  if (found_summaries != this->value_to_struct_summaries.end()) {
    return found_summaries->second;
  }

  /*
   * Determine the FieldSummaries this readReference could be accessing.
   */
  auto field_arg = call_inst.getArgOperand(0);
  auto &field_summaries = this->getFieldSummaries(*field_arg);

  /*
   * Create a ReferencedObjectSummary for each field accessed by this
   * readReference. These will be reconciled later.
   */
  auto value_struct_summaries = this->value_to_struct_summaries[&call_inst];
  for (auto field_summary : field_summaries) {
    auto struct_summary = new ReferencedObjectSummary(call_inst, field_summary);

    this->reference_summaries[&call_inst][field_summary] = struct_summary;
    value_struct_summaries.insert(struct_summary);
  }

  return value_struct_summaries;
}

} // namespace llvm::memoir
