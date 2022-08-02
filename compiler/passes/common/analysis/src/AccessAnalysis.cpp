#include "common/analysis/AccessAnalysis.hpp"
#include "common/utility/Metadata.hpp"

namespace llvm::memoir {

AccessAnalysis::AccessAnalysis(Module &M) : M(M) {
  // Do nothing.
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

AccessSummary *AccessAnalysis::getAccessSummaryForCall(
    llvm::CallInst &call_inst) {
  errs() << call_inst << "\n";
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
     * If there is only one possible field, return a MUST ReadSummary.
     */
    if (field_summaries.size() == 1) {
      auto field_summary = *(field_summaries.begin());

      auto read_summary = new MustReadSummary(call_inst, *field_summary);
      this->access_summaries[&call_inst] = read_summary;

      return read_summary;
    }

    /*
     * Get the TypeSummary of the first field.
     */
    auto first_field_summary = *(field_summaries.begin());
    auto &type_summary = (*first_field_summary).getType();

    /*
     * If there are more than one possible fields, return a MAY ReadSummary.
     */
    auto may_read_summary =
        new MayReadSummary(call_inst, type_summary, field_summaries);
    this->access_summaries[&call_inst] = may_read_summary;

    return may_read_summary;
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
     * If there is only one possible field, return a MUST WriteSummary.
     */
    if (field_summaries.size() == 1) {
      auto field_summary = *(field_summaries.begin());

      auto write_summary =
          new MustWriteSummary(call_inst, *field_summary, *value_written);
      this->access_summaries[&call_inst] = write_summary;

      return write_summary;
    }

    /*
     * Get the TypeSummary of the first field.
     */
    auto first_field_summary = *(field_summaries.begin());
    auto &type_summary = (*first_field_summary).getType();

    /*
     * If there are more than one possible fields, return a MAY WriteSummary.
     */
    auto may_write_summary = new MayWriteSummary(call_inst,
                                                 type_summary,
                                                 field_summaries,
                                                 *value_written);
    this->access_summaries[&call_inst] = may_write_summary;

    return may_write_summary;
  }

  /*
   * If we fell through for whatever reason, return NULL.
   */
  return nullptr;
}

set<FieldSummary *> &AccessAnalysis::getFieldSummaries(llvm::Value &value) {
  /*
   * Check if we have a memoized set of field summaries for this value.
   * If we do, return it.
   */
  auto found_summaries = this->field_summaries.find(&value);
  if (found_summaries != this->field_summaries.end()) {
    return found_summaries->second;
  }

  /*
   * If this is a call instruction either
   *  - Get the field summaries if its a MemOIR call
   *  - Get the field summaries for the function's return values
   */
  if (auto call_inst = dyn_cast<CallInst>(&value)) {
    auto callee = call_inst->getCalledFunction();

    // TODO
    if (callee == nullptr) {
      auto &empty_summaries = this->field_summaries[&value];
      empty_summaries.clear();
      return empty_summaries;
    }

    if (isMemOIRCall(*callee)) {
      return this->getFieldSummariesForCall(*call_inst);
    }

    set<llvm::Value *> return_values;
    for (auto &BB : *callee) {
      auto terminator = BB.getTerminator();
      if (auto return_inst = dyn_cast<ReturnInst>(terminator)) {
        auto return_value = return_inst->getReturnValue();
        return_values.insert(return_value);
      }
    }

    auto &call_field_summaries = this->field_summaries[&value];
    for (auto return_value : return_values) {
      assert(return_value
             && "in AccessAnalysis::getFieldSummaries"
                "return value is NULL");
      auto &return_field_summaries = this->getFieldSummaries(*return_value);
      call_field_summaries.insert(return_field_summaries.begin(),
                                  return_field_summaries.end());
    }

    return call_field_summaries;
  }

  /*
   * Handle PHI node
   */
  if (auto phi_node = dyn_cast<PHINode>(&value)) {
    auto &phi_field_summaries = this->field_summaries[&value];
    for (auto &incoming_use : phi_node->incoming_values()) {
      auto incoming_value = incoming_use.get();
      assert(incoming_value
             && "in AccessAnalysis::getFieldSummaries"
                "incoming value to PHI node is NULL");

      auto &incoming_field_summaries = this->getFieldSummaries(*incoming_value);
      phi_field_summaries.insert(incoming_field_summaries.begin(),
                                 incoming_field_summaries.end());
    }
    return phi_field_summaries;
  }

  /*
   * Handle lossless cast instructions
   */
  if (auto cast_inst = dyn_cast<CastInst>(&value)) {
    /*
     * If the cast instruction is not lossless, return the empty set.
     */
    if (!cast_inst->isLosslessCast()) {
      errs() << "lossy cast!\n";
      auto &cast_field_summaries = this->field_summaries[&value];
      cast_field_summaries.clear();
      return cast_field_summaries;
    }

    auto cast_value = cast_inst->getOperand(0);
    assert(cast_value
           && "in AccessAnalysis::getFieldSummaries"
              "value to be casted by cast inst is NULL");

    return this->getFieldSummaries(*cast_value);
  }

  /* TODO
   * Handle load inst
   */

  /* TODO
   * Handle argument
   */

  /*
   * Otherwise, this value has no field summaries, return the empty set.
   */
  auto &field_summaries = this->field_summaries[&value];
  field_summaries.clear();
  return field_summaries;
}

set<FieldSummary *> &AccessAnalysis::getFieldSummariesForCall(
    llvm::CallInst &call_inst) {
  /*
   * Look up the call instruction to see if we have already created an
   *   AccessSummary for it.
   */
  auto found_summary = this->field_summaries.find(&call_inst);
  if (found_summary != this->field_summaries.end()) {
    return found_summary->second;
  }

  /*
   * Create the Field Summary/ies for the call instruction.
   */
  auto callee = call_inst.getCalledFunction();

  /*
   * If the callee is an indirect call, then return a nullptr
   * We don't handle indirect calls at the moment as they should
   *   be statically resolved.
   */
  if (callee == nullptr) {
    auto &field_summaries = this->field_summaries[&call_inst];
    field_summaries.clear();
    return field_summaries;
  }

  auto callee_enum = getMemOIREnum(*callee);

  /*
   * Build the Field Summary/ies for the given MemOIR function
   * If this is not a call to a MemOIR field, then return the empty set.
   */
  switch (callee_enum) {
    case MemOIR_Func::GET_STRUCT_FIELD:
      return this->getStructFieldSummaries(call_inst);
    case MemOIR_Func::GET_TENSOR_ELEMENT:
      return this->getTensorElementSummaries(call_inst);
    default:
      auto &field_summaries = this->field_summaries[&call_inst];
      field_summaries.clear();
      return field_summaries;
  }
}

set<FieldSummary *> &AccessAnalysis::getStructFieldSummaries(
    llvm::CallInst &call_inst) {
  errs() << call_inst << "\n";
  /*
   * Determine the possible Allocation's that this field belongs to.
   */
  auto allocation_arg = call_inst.getArgOperand(0);
  assert(allocation_arg
         && "in AccessAnalysis::getStructFieldSummaries"
            "LLVM value being passed into getStructField is NULL");
  auto &object_summaries = this->getObjectSummaries(*allocation_arg);
  assert(!object_summaries.empty()
         && "in AccessAnalysis::getStructFieldSummaries"
            "no object summaries found");

  /*
   * Determine the field index being accessed.
   */
  auto field_index_arg = call_inst.getArgOperand(1);
  auto field_index_constant = dyn_cast<ConstantInt>(field_index_arg);
  assert(field_index_constant
         && "in AccessAnalysis::getStructFieldSummaries"
            "field index for getStructField is not constant");

  auto field_index = field_index_constant->getZExtValue();

  /*
   * Build the field summaries for the given struct field access
   */
  auto &field_summaries = this->field_summaries[&call_inst];
  for (auto object_summary : object_summaries) {
    auto field_summary =
        new StructFieldSummary(call_inst, *object_summary, field_index);
    field_summaries.insert(field_summary);
  }

  return field_summaries;
}

set<FieldSummary *> &AccessAnalysis::getTensorElementSummaries(
    llvm::CallInst &call_inst) {
  /*
   * Determine the possible allocations that this tensor element belongs to.
   */
  auto allocation_arg = call_inst.getArgOperand(0);
  assert(allocation_arg
         && "in AccessAnalysis::getTensorElementSummaries"
            "LLVM value being passed into getTensorElement is NULL");

  auto object_summaries = this->getObjectSummaries(*allocation_arg);
  assert(!object_summaries.empty()
         && "in AccessAnalysis::getTensorElementSummaries"
            "no object summaries found");

  /*
   * Determine the indices being accessed.
   */
  std::vector<llvm::Value *> indices(call_inst.arg_begin(),
                                     call_inst.arg_end());

  /*
   * Build the field summaries for the given tensor element access.
   */
  auto &field_summaries = this->field_summaries[&call_inst];
  for (auto object_summary : object_summaries) {
    auto field_summary =
        new TensorElementSummary(call_inst, *object_summary, indices);
    field_summaries.insert(field_summary);
  }

  return field_summaries;
}

set<ObjectSummary *> &AccessAnalysis::getObjectSummaries(llvm::Value &value) {
  /*
   * See if we have a memoized set of ObjectSummaries for this LLVM Value.
   *  - If we do, return it.
   *  - Otherwise, we will build them.
   */
  auto found_summaries = this->object_summaries.find(&value);
  if (found_summaries != this->object_summaries.end()) {
    errs() << "found memoized\n";
    return found_summaries->second;
  }

  /*
   * Check if the value is a call to readStruct or readTensor
   *  - If it is, fetch the NestedStructSummaries for it.
   */
  if (auto call_inst = dyn_cast<CallInst>(&value)) {
    auto callee_enum = getMemOIREnum(*call_inst);

    switch (callee_enum) {
      case MemOIR_Func::READ_STRUCT:
        return this->getReadStructSummaries(*call_inst);
      case MemOIR_Func::READ_TENSOR:
        return this->getReadTensorSummaries(*call_inst);
      default:
        break;
    }
  }

  /*
   * Check if the value is a lossless cast.
   *  - If it is, recurse on its value
   */
  if (auto cast_inst = dyn_cast<CastInst>(&value)) {
    auto &cast_object_summaries = this->object_summaries[&value];

    /*
     * If this is a lossy cast, return the empty set.
     */
    if (!cast_inst->isLosslessCast()) {
      cast_object_summaries.clear();
      return cast_object_summaries;
    }

    /*
     * Otherwise, recurse on the operand to the cast instruction.
     */
    auto operand_value = cast_inst->getOperand(0);
    assert(operand_value
           && "in AccessAnalysis::getObjectSummaries"
              "operand for CastInst is NULL");

    auto &operand_object_summaries = this->getObjectSummaries(*operand_value);
    cast_object_summaries.insert(operand_object_summaries.begin(),
                                 operand_object_summaries.end());

    return cast_object_summaries;
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
  auto &object_summaries = this->object_summaries[&value];
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
  errs() << call_inst << "\n";
  /*
   * See if we have a memoized set of ObjectSummaries for this CallInst.
   *  - If we do, return it.
   *  - Otherwise, create the NestedStructSummaries.
   */
  auto found_summaries = this->nested_object_summaries.find(&call_inst);
  if (found_summaries != this->nested_object_summaries.end()) {
    return found_summaries->second;
  }

  /*
   * Determine the FieldSummaries this readStruct could be accessing.
   */
  auto field_arg = call_inst.getArgOperand(0);
  auto &field_summaries = this->getFieldSummaries(*field_arg);
  errs() << "size: " << field_summaries.size() << "\n";

  /*
   * Build NestedStructSummaries for the given FieldSummaries.
   */
  auto &nested_object_summaries = this->nested_object_summaries[&call_inst];
  auto &object_summaries = this->object_summaries[&call_inst];
  for (auto field_summary : field_summaries) {
    /*
     * Create the new NestedObjectSummary
     */
    auto object_summary = new NestedStructSummary(call_inst, *field_summary);

    /*
     * Store the owned NestedObjectSummary pointer
     */
    nested_object_summaries.insert(object_summary);

    /*
     * Store the shared NestedObjectSummary pointer
     */
    object_summaries.insert(object_summary);
  }

  return object_summaries;
}

set<ObjectSummary *> &AccessAnalysis::getReadTensorSummaries(
    llvm::CallInst &call_inst) {
  /*
   * Constant length tensors are currently unsupported,
   *   readTensor call is current unsupported, always returns an empty set.
   */
  auto &nested_object_summary = this->nested_object_summaries[&call_inst];
  nested_object_summary.clear();
  return nested_object_summary;
}

bool AccessAnalysis::isRead(MemOIR_Func func_enum) {
  switch (func_enum) {
    case MemOIR_Func::READ_INTEGER:
    case MemOIR_Func::READ_UINT64:
    case MemOIR_Func::READ_UINT32:
    case MemOIR_Func::READ_UINT16:
    case MemOIR_Func::READ_UINT8:
    case MemOIR_Func::READ_INT64:
    case MemOIR_Func::READ_INT32:
    case MemOIR_Func::READ_INT16:
    case MemOIR_Func::READ_INT8:
    case MemOIR_Func::READ_FLOAT:
    case MemOIR_Func::READ_DOUBLE:
    case MemOIR_Func::READ_REFERENCE:
    case MemOIR_Func::READ_STRUCT:
    case MemOIR_Func::READ_TENSOR:
      return true;
    default:
      return false;
  }
}

bool AccessAnalysis::isWrite(MemOIR_Func func_enum) {
  switch (func_enum) {
    case MemOIR_Func::WRITE_INTEGER:
    case MemOIR_Func::WRITE_UINT64:
    case MemOIR_Func::WRITE_UINT32:
    case MemOIR_Func::WRITE_UINT16:
    case MemOIR_Func::WRITE_UINT8:
    case MemOIR_Func::WRITE_INT64:
    case MemOIR_Func::WRITE_INT32:
    case MemOIR_Func::WRITE_INT16:
    case MemOIR_Func::WRITE_INT8:
    case MemOIR_Func::WRITE_FLOAT:
    case MemOIR_Func::WRITE_DOUBLE:
    case MemOIR_Func::WRITE_REFERENCE:
      return true;
    default:
      return false;
  }
}

bool AccessAnalysis::isAccess(MemOIR_Func func_enum) {
  switch (func_enum) {
    case MemOIR_Func::READ_INTEGER:
    case MemOIR_Func::READ_UINT64:
    case MemOIR_Func::READ_UINT32:
    case MemOIR_Func::READ_UINT16:
    case MemOIR_Func::READ_UINT8:
    case MemOIR_Func::READ_INT64:
    case MemOIR_Func::READ_INT32:
    case MemOIR_Func::READ_INT16:
    case MemOIR_Func::READ_INT8:
    case MemOIR_Func::READ_FLOAT:
    case MemOIR_Func::READ_DOUBLE:
    case MemOIR_Func::READ_REFERENCE:
    case MemOIR_Func::READ_STRUCT:
    case MemOIR_Func::READ_TENSOR:
    case MemOIR_Func::WRITE_INTEGER:
    case MemOIR_Func::WRITE_UINT64:
    case MemOIR_Func::WRITE_UINT32:
    case MemOIR_Func::WRITE_UINT16:
    case MemOIR_Func::WRITE_UINT8:
    case MemOIR_Func::WRITE_INT64:
    case MemOIR_Func::WRITE_INT32:
    case MemOIR_Func::WRITE_INT16:
    case MemOIR_Func::WRITE_INT8:
    case MemOIR_Func::WRITE_FLOAT:
    case MemOIR_Func::WRITE_DOUBLE:
    case MemOIR_Func::WRITE_REFERENCE:
      return true;
    default:
      return false;
  }
}

/*
 * Logistics
 */
void AccessAnalysis::invalidate() {
  this->access_summaries.clear();
  this->field_summaries.clear();

  return;
}

/*
 * Singleton
 */
AccessAnalysis &AccessAnalysis::get(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    return *(found_analysis->second);
  }

  auto new_analysis = new AccessAnalysis(M);
  AccessAnalysis::analyses[&M] = new_analysis;
  return *new_analysis;
}

void AccessAnalysis::invalidate(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis->second);
    analysis.invalidate();
  }

  return;
}

map<llvm::Module *, AccessAnalysis *> AccessAnalysis::analyses = {};

} // namespace llvm::memoir
