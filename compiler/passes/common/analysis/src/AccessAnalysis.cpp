#include "common/analysis/AccessAnalysis.hpp"
#include "common/support/Metadata.hpp"

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
         * Build the AccessSummary for this call instruction.
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

  auto callee_name = callee->getName();
  auto callee_enum = getMemOIREnum(callee_name);

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
      auto const &field_summary = *(field_summaries.begin());
      auto read_summary =
          new ReadSummary(call_inst, PointsToInfo::MUST, field_summary);
      this->access_summaries[&call_inst] = read_summary;

      return *read_summary;
    }

    /*
     * If there are more than one possible fields, return a MAY ReadSummary.
     *  - First, build the read summaries.
     *  - Then, build the may read summary from the union of all read summaries.
     */
    set<ReadSummary *> may_read_summaries;
    for (auto const &field_summary : field_summaries) {
      auto read_summary =
          new ReadSummary(call_inst, PointsToInfo::MAY, *field_summary);
      may_read_summaries.insert(read_summary);
    }

    auto may_read_summary = new MayReadSummary(call_inst, may_read_summaries);
    this->access_summaries[&call_inst] = may_read_summary;

    return *may_read_summary;
  }

  /*
   * If the access is a write, create a WriteSummary for it.
   */
  if (isWrite(callee_enum)) {
    /*
     * If there is only one possible field, return a MUST WriteSummary.
     */
    if (field_summaries.size() == 1) {
      auto const &field_summary = *(field_summaries.begin());
      auto write_summary =
          new WriteSummary(call_inst, PointsToInfo::MUST, field_summary);
      this->access_summaries[&call_inst] = write_summary;

      return *read_summary;
    }

    /*
     * If there are more than one possible fields, return a MAY WriteSummary.
     *  - First, build the write summaries.
     *  - Then, build the may write summary from the union of all write
     *    summaries.
     */
    set<WriteSummary *> may_write_summaries;
    for (auto const &field_summary : field_summaries) {
      auto write_summary =
          new WriteSummary(call_inst, PointsToInfo::MAY, *field_summary);
      may_write_summaries.insert(write_summary);
    }

    auto may_write_summary =
        new MayWriteSummary(call_inst, may_write_summaries);
    this->access_summaries[&call_inst] = may_write_summary;

    return *may_write_summary;
  }

  /*
   * If we fell through for whatever reason, return NULL.
   */
  return nullptr;
}

/*
 * Internal helper functions
 */
set<FieldSummary *> &AccessAnalysis::getFieldSummaries(
    llvm::CallInst &call_inst) {
  /*
   * Check if we have a memoized set of field summaries for this call
   * instruction. If we do, return it.
   */
  auto found_summaries = this->field_summaries.find(&call_inst);
  if (found_summaries != this->field_summaries.end()) {
    return found_summaries.second;
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
    return nullptr;
  }

  auto callee_name = callee->getName();
  auto callee_enum = getMemOIREnum(callee_name);

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
  /*
   * Determine the possible Allocation's that this field belongs to.
   */
  auto allocation_arg = call_inst.getArgOperand(0);
  assert(allocation_arg
         && "in AccessAnalysis::getStructFieldSummaries"
            "LLVM value being passed into getStructField is NULL");

  auto &allocation_analysis = AllocationAnalysis::get(M);
  auto &allocation_summaries =
      allocation_analysis.getAllocationSummaries(*allocation_arg);

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
  for (auto allocation_summary : allocation_summaries) {
    auto field_summary =
        new StructFieldSummary(call_inst, *allocation_summary, field_index);
    field_summaries.insert(field_summary);
  }
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

  auto &allocation_analysis = AllocationAnalysis::get(M);
  auto &allocation_summaries =
      allocation_analysis.getAllocationSummaries(*allocation_arg);

  /*
   * Determine the indices being accessed.
   */
  std::vector<llvm::Value *> indices;
  indices.insert(call_inst.arg_begin(), call_inst.arg_end());

  /*
   * Build the field summaries for the given tensor element access.
   */
  auto &field_summaries = this->field_summaries[&call_inst];
  for (auto allocation_summary : allocation_summaries) {
    auto field_summary =
        new TensorElementSummary(call_inst, *allocation_summary, indices);
  }
}

set<AllocationSummary *> &AccessAnalysis::getAllocationSummaries(
    llvm::Value &value) {
  /*
   * Check if we have a memoized set of Allocation Summaries for the
   * given LLVM value. If we do, return it.
   */
  auto found_summaries = this->allocation_summaries.find(&value);
  if (found_summaries != this->allocation_summaries.end()) {
    return found_summaries.second;
  }

  /*
   * Build the allocation summaries for the given LLVM Value.
   */

  /*
   * If we have a call instruction, either
   *  - It is a call to a MemOIR allocation.
   *  - It is a call to a program function, in which case perform
   *      interprocedural analysis.
   *  - It is an empty/intrinsic function, in which case ERROR!
   */
  if (auto call_inst = dyn_cast<CallInst>(&value)) {
    auto &allocation_analysis = AllocationAnalysis::get(this->M);
    auto allocation_summary =
        allocation_analysis.getAllocationSummary(*call_inst);

    /*
     * If this call instruction is an allocation summary, then it must alias.
     * Return a set containing only this allocation summary.
     */
    if (allocation_summary) {
      this->allocation_summaries[&value].insert(allocation_summary);
      return this->allocation_summaries[&value];
    }

    /*
     * Get the callee, sanity check that it is a direct call and non-empty.
     *
     * TODO: add support for indirect calls, relies on NOELLE for the call graph
     * TODO: add support for intrinsics and library calls that return a MemOIR
     *       object, requires knowledge about these functions.
     */
    auto callee = call_inst->getCalledFunction();
    assert(callee
           && "in AccessAnalysis::getAllocationSummaries"
              "indirect callee, currently unsupported!");

    assert((!callee->empty())
           && "in AccessAnalysis::getAllocationSummaries"
              "callee is empty (library or instrinsic) currently unsupported!");

    /*
     * Otherwise, we will analyze the function.
     * Fetch the return values from the callee.
     */
    set<llvm::Value *> callee_return_values = {};
    for (auto &BB : *callee) {
      auto terminator = BB.getTerminator();
      if (auto return_inst = dyn_cast<ReturnInst>(terminator)) {
        auto return_value = return_inst->getReturnValue();
        assert(return_value
               && "in AccessAnalysis::getAllocationSummaries"
                  "LLVM return value of function is NULL");
        callee_return_values.insert(return_value);
      }
    }

    /*
     * Then recurse into the callee return values, unioning the output set.
     */
    auto &callee_allocation_summaries = this->allocation_summaries[&value];
    for (auto callee_return_value : callee_return_values) {
      auto &return_allocation_summaries =
          this->getAllocationSummaries(*callee_return_value);
      callee_allocation_summaries.insert(return_allocation_summaries.cbegin(),
                                         return_allocation_summaries.cend());
    }

    /*
     * Return the callee allocation summaries.
     */
    return callee_allocation_summaries;
  }

  /*
   * If we have a PHI node
   *  - Recurse into each of the incoming values
   *  - Union the result of each incoming value
   */
  if (auto phi_node = dyn_cast<PHINode>(&value)) {
    /*
     * For each incoming value, calculate its allocation summaries.
     */
    auto &phi_allocation_summaries = this->allocation_summaries[&value];
    for (auto incoming_value : phi_node->incoming_values()) {
      assert(incoming_value
             && "in AccessAnalysis::getAllocationSummaries"
                "incoming value to PHINode is NULL");

      auto &incoming_allocation_summaries =
          this->getAllocationSummaries(*incoming_value);

      /*
       * Union all of the incoming allocation summaries.
       */
      phi_allocation_summaries.insert(incoming_allocation_summaries.cbegin(),
                                      incoming_allocation_summaries.cend());
    }
  }

  /*
   * If we have a load instruction
   *  - Assert that we are loading a GlobalVariable or an alloca
   *  - Look for all stores to the pointer (context sensitive)
   *  - Recurse into the value being loaded
   */
  if (auto load_inst = dyn_cast<LoadInst>(&value)) {
    /*
     * Get a reference to the load's allocation summaries set
     */
    auto &load_allocation_summaries = this->allocation_summaries[&value];

    /*
     * Find the pointer we are referencing.
     * Assert that it is a global variable or an alloca.
     *   If it isn't, error.
     */
    auto load_ptr = load_inst->getPointerOperand();

    /*
     * If the load instruction's pointer operand is a GEP, unroll it.
     */
    auto ptr_as_gep = dyn_cast<GetElementPtrInst>(load_ptr);
    if (ptr_as_gep) {
      assert(ptr_as_gep->hasAllZeroIndices()
             && "in AccessAnalysis::getAllocationSummaries"
                "GEP used in load has non-zero indices");
      load_ptr = ptr_as_gep->getPointerOperand();
    }

    /*
     * If the pointer is a GlobalVariable or Alloca, look for all stores to it.
     */
    if (isa<GlobalVariable>(load_ptr) || isa<AllocaInst>(load_ptr)) {
      for (auto user : global_variable->users()) {
        auto user_func = user->getFunction();
        assert(user_func
               && "in AccessAnalysis::getAllocationSummaries"
                  "user does not belong to a function");

        /*
         * Skip internal MemOIR functions.
         */
        if (MetadataManager::hasMetadata(*user_func, MetadataType::INTERNAL)) {
          continue;
        }

        /*
         * If the user is a GEP, unroll it and iterate on its users.
         */
        if (auto user_as_gep = dyn_cast<GetElementPtrInst>(user)) {
          assert(user_as_gep->hasAllZeroIndices()
                 && "in AccessAnalysis::getAllocationSummaries"
                    "GEP used in store has non-zero indices");

          for (auto gep_user : user_as_gep->users()) {
            if (auto gep_user_as_store = dyn_cast<StoreInst>(gep_user)) {
              auto store_value = gep_user_as_store->
            }
          }
        }

        /*
         * If the user is a store, recurse on the value being stored.
         */
        if (auto user_as_store = dyn_cast<StoreInst>(user)) {
          auto store_value = user_as_store->getValueOperand();
          assert(store_value
                 && "in AccessAnalysis::getAllocationSummaries"
                    "value being stored is NULL");

          auto &store_allocation_summaries =
              this->getAllocationSummaries(*store_value);

          /*
           * Union all of the allocation summaries from the store
           */
          load_allocation_summaries.insert(store_allocation_summaries.cbegin(),
                                           store_allocation_summaries.cend());
        }
      }
    }

    /*
     * Return the Load's allocation summaries
     */
    return load_allocation_summaries;
  }

  /*
   * If we have a select instruction, recurse on its true and false values.
   */
  if (auto select_inst = dyn_cast<SelectInst>(&value)) {
    /*
     * Recurse on the true value of the select instruction.
     */
    auto true_value = select_inst->getTrueValue();
    assert(true_value
           && "in AllocationAnalysis::getAllocationSummaries"
              "true value for select instruction is NULL");
    auto &true_allocation_summaries = this->getAllocationSummaries(*true_value);

    /*
     * Recurse on the false value of the select instruction.
     */
    auto false_value = select_inst->getFalseValue();
    assert(false_value
           && "in AllocationAnalysis::getAllocationSummaries"
              "false value for select instruction is NULL");
    auto &false_allocation_summaries =
        this->getAllocationSummaries(*false_value);

    /*
     * Union the true and false allocation summaries.
     */
    auto &switch_allocation_summaries = this->allocation_summaries[&value];
    switch_allocation_summaries.insert(true_allocation_summaries.begin(),
                                       true_allocation_summaries.end());
    switch_allocation_summaries.insert(false_allocation_summaries.begin(),
                                       false_allocation_summaries.end());

    /*
     * Return the select's allocation summaries
     */
    return switch_allocation_summaries;
  }
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

/*
 * Logistics
 */
void AccessAnalysis::invalidate() {
  this->access_summaries.clear();
  this->field_summaries.clear();
  this->allocation_summaries.clear();

  return;
}

/*
 * Singleton
 */
AccessAnalysis &AccessAnalysis::get(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    return *(found_analysis.second);
  }

  auto new_analysis = new AccessAnalysis(M);
  AccessAnalysis::analyses[&M] = new_analysis;
  return *new_analysis;
}

void AccessAnalysis::invalidate(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis.second);
    analysis.invalidate();
    delete &analysis;
  }

  return;
}

} // namespace llvm::memoir
