#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

void AccessAnalysis::analyzeObjects() {
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
        getObjectSummaries(I);
      }
    }
  }

  return;
}

void AccessAnalysis::reconcileReferences() {
  errs() << "AccessAnalysis: reconciling references.\n";

  /*
   * Gather all reference summaries that still need to be reconciled.
   */
  set<ReferencedObjectSummary *> worklist;
  for (auto const &[call_inst, field_map] : this->reference_summaries) {
    for (auto const &[reference_field, referenced_object] : field_map) {
      worklist.insert(referenced_object);
    }
  }

  /*
   * For each temporary reference summary, reconcile it and its fields.
   * Do an interprocedural analysis to find the possible references.
   */
  while (!worklist.empty) {
    set<ReferencedObjectSummary *> reconciled_references;
    for (auto referenced_object : worklist) {
      bool is_reconcilable = true;

      /*
       * Find all possible references that could be read here.
       * TODO: make this flow-sensitive.
       */
      set<ObjectSummary *> objects_written;
      auto &reference_field = referenced_object->getField();
      auto &field_accesses = this->getFieldAccesses(*reference_field);
      for (auto field_access : field_accesses) {

        /*
         * Don't need to consider reads.
         */
        if (field_access->isRead()) {
          continue;
        }

        /*
         * Get the value written and fetch the ObjectSummaries for it.
         */
        auto &write_access = static_cast<WriteSummary &>(*field_access);
        auto &value_written = write_access.getValueWritten();
        auto &objects_written_by_access =
            this->getObjectSummaries(value_written);

        /*
         * If the objects being written contains a ReferenceObjectSummary,
         *   continue, it will be resolved later.
         * Otherwise, add the object to the set of objects written to the
         *   reference field for reconciliation later.
         */
        bool has_reference = false;
        for (auto object_written : objects_written_by_access) {
          auto object_code = object_written->getCode();
          if (object_code == ObjectCode::REFERENCE) {
            is_reconcilable &= false;
            break;
          }

          objects_written.insert(object_written);
        }

        if (!is_reconcilable) {
          break;
        }
      }

      if (is_reconcilable) {
        /*
         * Get the field summaries that the referenced object reconciles to.
         */
        for (auto object_written : objects_written) {
        }

        /*
         * Reconcile all accesses to fields.
         */
        for (auto field : this->fields_to_reconcile[reference_object]) {
          auto &field_accesses = this->getFieldAccesses(field);
          for (auto access : field_accesses) {
          }
        }

        /*
         * Mark the reconciled reference for deletion.
         */
        reconciled_references.insert(referenced_object);
      }
    }

    /*
     * Delete the reconciled references.
     */
    worklist.erase(reconciled_references.begin(), reconciled_references.end());
  }

  return;
}

set<ObjectSummary *> &AccessAnalysis::getObjectSummaries(llvm::Value &value) {
  /*
   * See if we have a memoized set of ObjectSummaries for this LLVM Value.
   *  - If we do, return it.
   *  - Otherwise, we will build them.
   */
  auto found_summaries = this->value_to_object_summaries.find(&value);
  if (found_summaries != this->value_to_object_summaries.end()) {
    return found_summaries->second;
  }

  /*
   * Initialize the object to summaries for this value.
   */
  auto &object_summaries = this->value_to_object_summaries[&value];
  object_summaries.clear();

  /*
   * Check if the value is a call to readStruct, readTensor or readReference
   *  - If it is, fetch the object summaries for it.
   */
  if (auto call_inst = dyn_cast<CallInst>(&value)) {
    auto callee_enum = getMemOIREnum(*call_inst);

    switch (callee_enum) {
      case MemOIR_Func::READ_STRUCT: {
        return this->getReadStructSummaries(*call_inst);
      }
      case MemOIR_Func::READ_TENSOR: {
        return this->getReadTensorSummaries(*call_inst);
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
      object_summaries.clear();
      return object_summaries;
    }

    /*
     * Otherwise, recurse on the operand to the cast instruction.
     */
    auto operand_value = cast_inst->getOperand(0);
    assert(operand_value
           && "in AccessAnalysis::getObjectSummaries"
              "operand for CastInst is NULL");

    auto &operand_object_summaries = this->getObjectSummaries(*operand_value);
    object_summaries.insert(operand_object_summaries.begin(),
                            operand_object_summaries.end());

    return object_summaries;
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
    BaseObjectSummary *base_object_summary;
    auto found_summary = this->base_object_summaries.find(allocation_summary);
    if (found_summary != this->base_object_summaries.end()) {
      base_object_summary = found_summary->second;
    } else {
      base_object_summary = new BaseObjectSummary(*allocation_summary);

      this->base_object_summaries[allocation_summary] = base_object_summary;
    }

    object_summaries.insert(base_object_summary);
  }

  /*
   * Return a set of the ObjectSummaries
   */
  return object_summaries;
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
  auto &value_object_summaries = this->value_to_object_summaries[&call_inst];
  for (auto field_summary : field_summaries) {
    /*
     * See if we have a memoized set of ObjectSummaries for this FieldSummary.
     * If we do, add it to the set.
     */
    auto found_summary = this->nested_object_summaries.find(field_summary);
    if (found_summary != this->nested_object_summaries.end()) {
      auto object_summary = found_summary->second;
      value_object_summaries.insert(object_summary);
      continue;
    }

    /*
     * Create the new NestedObjectSummary.
     */
    auto object_summary = new NestedObjectSummary(*field_summary);

    /*
     * Store the owned NestedObjectSummary pointer.
     */
    nested_object_summaries[field_summary] = object_summary;

    /*
     * Store the shared NestedObjectSummary pointer.
     */
    value_object_summaries.insert(object_summary);
  }

  /*
   * Return the set of object summaries.
   */
  return value_object_summaries;
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
  auto &object_summaries = this->value_to_object_summaries[&call_inst];
  for (auto field_summary : field_summaries) {
    /*
     * See if we have a memoized set of ObjectSummaries for this CallInst.
     * If we do, insert it into the set.
     */
    auto found_summary = this->nested_object_summaries.find(field_summary);
    if (found_summary != this->nested_object_summaries.end()) {
      auto object_summary = found_summary->second;
      object_summaries.insert(object_summary);
      continue;
    }

    /*
     * Otherwise, create the new NestedTensorSummary.
     */
    auto object_summary = new NestedObjectSummary(*field_summary);

    /*
     * Store the owned NestedObjectSummary pointer.
     */
    nested_object_summaries[field_summary] = object_summary;

    /*
     * Store the shared NestedObjectSummary pointer.
     */
    object_summaries.insert(object_summary);
  }

  /*
   * Return the object summaries.
   */
  return object_summaries;
}

set<ObjectSummary *> &AccessAnalysis::getReadReferenceSummaries(
    llvm::CallInst &call_inst) {
  /*
   * See if we have a memoized set of ObjectSummaries for this CallInst.
   *  - If we do, return it.
   *  - Otherwise, create the NestedStructSummaries.
   */
  auto found_summaries = this->value_to_object_summaries.find(&call_inst);
  if (found_summaries != this->value_to_object_summaries.end()) {
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
  auto value_object_summaries = this->value_to_object_summaries[&call_inst];
  for (auto field_summary : field_summaries) {
    auto object_summary = new ReferencedObjectSummary(call_inst, field_summary);

    this->reference_summaries[&call_inst][field_summary] = object_summary;
    value_object_summaries.insert(object_summary);
  }

  return value_object_summaries;
}

} // namespace llvm::memoir
