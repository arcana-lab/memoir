#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

void AccessAnalysis::analyzeFields() {
  errs() << "AccessAnalysis: analyzing fields.\n";

  /*
   * Compute the field summaries for the program.
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
        getFieldSummaries(I);
      }
    }
  }

  return;
}

set<FieldSummary *> &AccessAnalysis::getFieldSummaries(llvm::Value &value) {
  /*
   * Check if we have a memoized set of field summaries for this value.
   * If we do, return it.
   */
  auto found_summaries = this->value_to_field_summaries.find(&value);
  if (found_summaries != this->value_to_field_summaries.end()) {
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
      auto &empty_summaries = this->value_to_field_summaries[&value];
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

    auto &call_field_summaries = this->value_to_field_summaries[&value];
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
    auto &phi_field_summaries = this->value_to_field_summaries[&value];
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
      auto &cast_field_summaries = this->value_to_field_summaries[&value];
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
  auto &field_summaries = this->value_to_field_summaries[&value];
  field_summaries.clear();
  return field_summaries;
}

set<FieldSummary *> &AccessAnalysis::getFieldSummariesForCall(
    llvm::CallInst &call_inst) {
  /*
   * Look up the call instruction to see if we have already created an
   *   AccessSummary for it.
   */
  auto found_summary = this->value_to_field_summaries.find(&call_inst);
  if (found_summary != this->value_to_field_summaries.end()) {
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
    auto &field_summaries = this->value_to_field_summaries[&call_inst];
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
      auto &field_summaries = this->value_to_field_summaries[&call_inst];
      field_summaries.clear();
      return field_summaries;
  }
}

set<FieldSummary *> &AccessAnalysis::getStructFieldSummaries(
    llvm::CallInst &call_inst) {
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
  auto &field_summaries = this->value_to_field_summaries[&call_inst];
  for (auto object_summary : object_summaries) {
    /*
     * Get the StructFieldSummary and memoize it.
     */
    auto &field_summary =
        fetchOrCreateStructFieldSummary(*object_summary, field_index);
    field_summaries.insert(&field_summary);
  }

  return field_summaries;
}

StructFieldSummary &AccessAnalysis::fetchOrCreateStructFieldSummary(
    ObjectSummary &object,
    uint64_t field_index) {
  /*
   * Check if we already have a memoized field summary.
   */
  auto found_fields = this->field_summaries.find(&object);
  if (found_fields != this->field_summaries.end()) {
    auto &fields = found_fields->second;
    for (auto field : fields) {
      auto &struct_field = static_cast<StructFieldSummary &>(*field);
      if (struct_field.getIndex() == field_index) {
        return struct_field;
      }
    }
  }

  /*
   * Otherwise, we will create the StructFieldSummary.
   */
  auto struct_field = new StructFieldSummary(object, field_index);

  /*
   * Memoize the StructFieldSummary.
   */
  this->field_summaries[&object].insert(struct_field);

  /*
   * Return the StructFieldSummary.
   */
  return *struct_field;
}

set<FieldSummary *> &AccessAnalysis::getTensorElementSummaries(
    llvm::CallInst &call_inst) {
  /*
   * Determine the possible allocations that this tensor element belongs to.
   */
  auto allocation_argument = call_inst.getArgOperand(0);
  assert(allocation_argument
         && "in AccessAnalysis::getTensorElementSummaries"
            "LLVM value being passed into getTensorElement is NULL");

  auto object_summaries = this->getObjectSummaries(*allocation_argument);
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
  auto &value_field_summaries = this->value_to_field_summaries[&call_inst];
  for (auto object_summary : object_summaries) {
    auto field_summary = new TensorElementSummary(*object_summary, indices);
    value_field_summaries.insert(field_summary);
    this->field_summaries[object_summary].insert(field_summary);
  }

  return value_field_summaries;
}

TensorElementSummary &AccessAnalysis::fetchOrCreateTensorElementSummary(
    ObjectSummary &object,
    vector<llvm::Value *> &indices) {
  /*
   * Check if we already have a memoized TensorElementSummary.
   * If we do, return it.
   */
  auto found_fields = this->field_summaries.find(&object);
  if (found_fields != this->field_summaries.end()) {
    auto &fields = found_fields->second;
    for (auto field : fields) {
      auto &tensor_element = static_cast<TensorElementSummary &>(*field);

      bool indices_equal = true;
      for (auto i = 0; i < tensor_element.getNumberOfDimensions(); i++) {
        auto &tensor_index = tensor_element.getIndex(i);
        if (indices.at(i) != &tensor_index) {
          indices_equal = false;
          break;
        }
      }

      if (indices_equal) {
        return tensor_element;
      }
    }
  }

  /*
   * Otherwise create the TensorElementSummary.
   */
  auto tensor_element = new TensorElementSummary(object, indices);

  /*
   * Memoize the TensorElementSummary.
   */
  this->field_summaries[&object].insert(tensor_element);

  /*
   * Return the TensorElementSummary.
   */
  return *tensor_element;
}

} // namespace llvm::memoir
