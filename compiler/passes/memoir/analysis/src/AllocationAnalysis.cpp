#include "memoir/analysis/AllocationAnalysis.hpp"
#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

AllocationAnalysis::AllocationAnalysis(Module &M) : M(M) {
  // Do nothing.
}

set<AllocationSummary *> &AllocationAnalysis::getAllocationSummaries(
    llvm::Value &value) {
  /*
   * Check if we have a memoized set of Allocation Summaries for the
   * given LLVM value. If we do, return it.
   */
  auto found_summaries = this->allocation_summaries.find(&value);
  if (found_summaries != this->allocation_summaries.end()) {
    return found_summaries->second;
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
    /*
     * If this call instruction is an allocation summary, then it must alias.
     * Return a set containing only this allocation summary.
     */
    auto allocation_summary = this->getAllocationSummary(*call_inst);
    if (allocation_summary) {
      auto &call_allocation_summaries = this->allocation_summaries[&value];
      call_allocation_summaries.insert(allocation_summary);
      return call_allocation_summaries;
    }

    /*
     * Get the callee, sanity check that it is a direct call and non-empty.
     *
     * TODO: add support for indirect calls, relies on NOELLE for the call
     * graph
     * TODO: add support for intrinsics and library calls that return a MemOIR
     *       object, requires knowledge about these functions.
     */
    auto callee = call_inst->getCalledFunction();
    assert(callee
           && "in AccessAnalysis::getAllocationSummaries"
              "indirect callee, currently unsupported!");

    if (MetadataManager::hasMetadata(*callee, MetadataType::INTERNAL)
        || callee->empty()) {
      auto &callee_allocation_summaries = this->allocation_summaries[&value];
      callee_allocation_summaries.clear();
      return callee_allocation_summaries;
    }

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
    for (auto &incoming_value : phi_node->incoming_values()) {
      assert(incoming_value
             && "in AllocationAnalysis::getAllocationSummaries"
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
     * If the pointer is a GlobalVariable or Alloca, look for all stores to
     * it.
     */
    if (isa<GlobalVariable>(load_ptr) || isa<AllocaInst>(load_ptr)) {
      for (auto user : load_ptr->users()) {
        auto user_inst = dyn_cast<Instruction>(user);
        assert(user_inst
               && "in AccessAnalysis::getAllocationSummaries"
                  "user is not an instruction");
        auto user_func = user_inst->getFunction();
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
              auto store_value = gep_user_as_store->getValueOperand();
              assert(store_value
                     && "in AccessAnalysis::getAllocationSummaries"
                        "value being stored to gep is NULL");

              auto &store_allocation_summaries =
                  this->getAllocationSummaries(*store_value);

              /*
               * Union all of the allocation summaries from the store.
               */
              load_allocation_summaries.insert(
                  store_allocation_summaries.cbegin(),
                  store_allocation_summaries.cend());
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
                    "value being stored to pointer is NULL");

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

  /*
   * If we have a cast instruction:
   *  - If it is lossless, return the empty set
   *  - Otherwise, recurse on its operand
   */
  if (auto cast_inst = dyn_cast<CastInst>(&value)) {
    auto &cast_allocation_summaries = this->allocation_summaries[&value];

    /*
     * If this is a lossy cast, return the empty set.
     */
    if (!cast_inst->isLosslessCast()) {
      cast_allocation_summaries.clear();
      return cast_allocation_summaries;
    }

    /*
     * Otherwise, recurse on the operand to the cast instruction.
     */
    auto operand_value = cast_inst->getOperand(0);
    assert(operand_value
           && "in AllocationAnalysis::getAllocationSummaries"
              "operand for CastInst is NULL");

    auto &operand_allocation_summaries =
        this->getAllocationSummaries(*operand_value);
    cast_allocation_summaries.insert(operand_allocation_summaries.begin(),
                                     operand_allocation_summaries.end());

    return cast_allocation_summaries;
  }

  /*
   * If we have a function argument, recurse on its caller args.
   */
  if (auto argument = dyn_cast<Argument>(&value)) {

    /*
     * Collect all callers of this function in the program.
     */
    set<CallInst *> callers = {};
    for (auto &F : M) {
      if (MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
        continue;
      }

      if (F.empty()) {
        continue;
      }

      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto call_inst = dyn_cast<CallInst>(&I)) {

            /*
             * If this is an indirect call, check if its function type matches
             * that of the argument's parent. If it does, it may call it.
             */
            if (call_inst->isIndirectCall()) {
              auto caller_function_type = call_inst->getFunctionType();
              auto callee = argument->getParent();
              auto callee_function_type = callee->getFunctionType();

              if (caller_function_type == callee_function_type) {
                callers.insert(call_inst);
              }

              continue;
            }

            /*
             * If this is not an indirect call, get its callee and check if it's
             * the argument's parent function.
             */
            auto callee = call_inst->getCalledFunction();
            if (callee == argument->getParent()) {
              callers.insert(call_inst);
            }
          }
        }
      }
    }

    /*
     * For each caller, iterate on its argument.
     */
    auto &argument_allocation_summaries = this->allocation_summaries[&value];
    auto argument_index = argument->getArgNo();
    for (auto caller : callers) {
      auto caller_argument = caller->getArgOperand(argument_index);
      assert(caller_argument
             && "in AllocationAnalysis::getAllocationSummary"
                "argument to caller is NULL!");

      /*
       * Recurse on the caller's argument(s)
       */
      auto &caller_allocation_summaries =
          this->getAllocationSummaries(*caller_argument);

      /*
       * Union the caller's allocation summaries.
       */
      argument_allocation_summaries.insert(caller_allocation_summaries.begin(),
                                           caller_allocation_summaries.end());
    }

    /*
     * Return the allocation summaries for the argument
     */
    return argument_allocation_summaries;
  }

  /*
   * Otherwise, this can't be an allocation summary.
   * Return NULL.
   */
  auto &value_allocation_summaries = this->allocation_summaries[&value];
  value_allocation_summaries.clear();
  return value_allocation_summaries;
}

AllocationSummary *AllocationAnalysis::getAllocationSummary(
    CallInst &call_inst) {
  /*
   * Look up the call instruction to see if we have a
   *   memoized AllocationSummary.
   */
  auto found_summary = this->the_allocation_summaries.find(&call_inst);
  if (found_summary != this->the_allocation_summaries.end()) {
    return found_summary->second;
  }

  /*
   * If the call instruction is not memoized, then
   *   we need to create its AllocationSummary.
   */
  auto callee = call_inst.getCalledFunction();

  /*
   * If the callee is an indirect call, then return a nullptr.
   * We don't handle indirect calls at the moment as they should
   *   be statically resolved.
   */
  if (callee == nullptr) {
    return nullptr;
  }

  auto callee_enum = getMemOIREnum(*callee);

  /*
   * Build the AllocationSummary for the given MemOIR allocation call.
   */
  AllocationSummary *allocation_summary;
  switch (callee_enum) {
    case MemOIR_Func::ALLOCATE_STRUCT:
      allocation_summary = this->getStructAllocationSummary(call_inst);
      break;
    case MemOIR_Func::ALLOCATE_TENSOR:
      allocation_summary = this->getTensorAllocationSummary(call_inst);
      break;
    default:
      return nullptr;
  }

  /*
   * Memoize the AllocationSummary we just built.
   */
  this->the_allocation_summaries[&call_inst] = allocation_summary;

  /*
   * Return the AllocationSummary.
   */
  return allocation_summary;
}

AllocationSummary *AllocationAnalysis::getStructAllocationSummary(
    CallInst &call_inst) {
  /*
   * Get the Type value from the call.
   */
  auto type_value = call_inst.getArgOperand(0);
  assert(type_value && "in AllocationAnalysis::getStructAllocationSummary"
         && "type argument to call instruction is null");

  /*
   * Determine the type summary.
   */
  auto &type_analysis = TypeAnalysis::get(this->M);
  auto type_summary = type_analysis.getTypeSummary(*type_value);
  assert(type_summary && "in AllocationAnalysis::getStructAllocationSummary"
         && "type summary for struct allocation not found");

  /*
   * Create the allocation.
   */
  return new StructAllocationSummary(call_inst, *type_summary);
}

AllocationSummary *AllocationAnalysis::getTensorAllocationSummary(
    CallInst &call_inst) {
  /*
   * Get the Type value from the call.
   */
  auto type_value = call_inst.getArgOperand(0);
  assert(type_value && "in AllocationAnalysis::getTensorAllocationSummary"
         && "type argument to call instruction is null");

  /*
   * Determine the type summary.
   */
  auto &type_analysis = TypeAnalysis::get(this->M);
  auto element_type_summary = type_analysis.getTypeSummary(*type_value);
  assert(element_type_summary
         && "in AllocationAnalysis::getTensorAllocationSummary"
         && "element type summary for tensor not found");

  /*
   * Get the number of dimensions.
   */
  auto num_dimensions_value = call_inst.getArgOperand(1);
  auto num_dimensions_constant = dyn_cast<ConstantInt>(num_dimensions_value);
  assert(num_dimensions_constant
         && "in AllocationAnalysis::getTensorAllocationSummary"
         && "number of dimensions for tensor allocation is not constant");
  auto num_dimensions = num_dimensions_constant->getZExtValue();

  /*
   * Get the length of dimensions.
   */
  std::vector<Value *> length_of_dimensions;
  for (auto i = 0; i < num_dimensions; i++) {
    auto dimension_arg_idx = i + 2;
    auto dimension_length = call_inst.getArgOperand(dimension_arg_idx);
    length_of_dimensions.push_back(dimension_length);
  }

  /*
   * Create the allocation
   */
  return new TensorAllocationSummary(call_inst,
                                     *element_type_summary,
                                     length_of_dimensions);
}

AllocationSummary *AllocationAnalysis::getAssocArrayAllocationSummary(
    CallInst &call_inst) {
  /*
   * TODO
   */

  return nullptr;
}

AllocationSummary *AllocationAnalysis::getSequenceAllocationSummary(
    CallInst &call_inst) {
  /*
   * TODO
   */

  return nullptr;
}

/*
 * Logistics
 */
void AllocationAnalysis::invalidate() {
  this->allocation_summaries.clear();
  // TODO: free all of the_allocation_summaries
  this->the_allocation_summaries.clear();
  return;
}

/*
 * Singleton
 */
AllocationAnalysis &AllocationAnalysis::get(Module &M) {
  auto found_analysis = AllocationAnalysis::analyses.find(&M);
  if (found_analysis != AllocationAnalysis::analyses.end()) {
    return *(found_analysis->second);
  }

  auto analysis = new AllocationAnalysis(M);
  AllocationAnalysis::analyses[&M] = analysis;

  return *analysis;
}

void AllocationAnalysis::invalidate(Module &M) {
  auto found_analysis = AllocationAnalysis::analyses.find(&M);
  if (found_analysis != AllocationAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis->second);
    analysis.invalidate();
  }

  return;
}

map<llvm::Module *, AllocationAnalysis *> AllocationAnalysis::analyses = {};

} // namespace llvm::memoir
