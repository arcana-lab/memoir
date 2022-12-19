#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

TypeAnalysis::TypeAnalysis(Module &M) : M(M) {
  // Do nothing.
}

Type *TypeAnalysis::getType(llvm::Value &V) {
  /*
   * Trace back the value to find the associated
   *   Type, if it exists.
   */

  /*
   * If we have a call instruction,
   *  - get its Type, if it is a MemOIR call, return it.
   *  - otherwise, we need to recurse on the function callee.
   */
  if (auto call_inst = dyn_cast<CallInst>(&V)) {
    auto const &type_summary = this->getMemOIRType(*call_inst);
    if (type_summary) {
      return type_summary;
    }

    /*
     * Get the callee, sanity check that it is a direct call and non-empty.
     *
     * TODO: add support for indirect call
     * TODO: add support for intrinsics and library calls, requires knowledge of
     *       how they work.
     */
    auto callee = call_inst->getCalledFunction();
    if (!callee) {
      return nullptr;
    }

    if (MetadataManager::hasMetadata(*callee, MetadataType::INTERNAL)) {
      return nullptr;
    }

    if (callee->empty()) {
      return nullptr;
    }

    /*
     * Fetch the return values from the callee.
     */
    set<llvm::Value *> callee_return_values = {};
    for (auto &BB : *callee) {
      auto terminator = BB.getTerminator();
      if (auto return_inst = dyn_cast<ReturnInst>(terminator)) {
        auto return_value = return_inst->getReturnValue();
        if (!return_value) {
          return nullptr;
        }

        callee_return_values.insert(return_value);
      }
    }

    /*
     * Recurse on the return values of the call instruction.
     */
    Type *call_type_summary = nullptr;
    for (auto callee_return_value : callee_return_values) {
      auto return_type_summary = this->getType(*callee_return_value);

      /*
       * If there is no Type for the return value, return NULL.
       */
      if (!return_type_summary) {
        return nullptr;
      }

      /*
       * Save the first Type we find.
       */
      if (!call_type_summary) {
        call_type_summary = return_type_summary;
        continue;
      }

      /*
       * If the return type summary and the call type summary are not the same,
       *   error. We must be statically typed.
       */
      if (!return_type_summary->equals(call_type_summary)) {
        return nullptr;
      }
    }

    /*
     * Return the call's Type.
     */
    return call_type_summary;
  }

  /*
   * If we have load instruction, trace back to its
   *   global variable and find the original store to it.
   */
  if (auto load_inst = dyn_cast<LoadInst>(&V)) {
    auto load_ptr = load_inst->getPointerOperand();

    auto global = dyn_cast<GlobalVariable>(load_ptr);
    if (!global) {
      if (auto load_gep = dyn_cast<GetElementPtrInst>(load_ptr)) {
        auto gep_ptr = load_gep->getPointerOperand();
        global = dyn_cast<GlobalVariable>(gep_ptr);
      }
    }

    /*
     * If we still cannot find the GlobalVariable, return NULL.
     */
    if (!global) {
      return nullptr;
    }

    /*
     * Find the original store for this global variable.
     */
    for (auto user : global->users()) {
      if (auto store_inst = dyn_cast<StoreInst>(user)) {
        auto store_value = store_inst->getValueOperand();
        auto store_call = dyn_cast<CallInst>(store_value);
        MEMOIR_NULL_CHECK(store_call,
                          "original store to type's global is not a call");

        return this->get_type_summary(*store_call);
      }

      // TODO: handle GEP's here, hasn't broken yet.
    }
  }

  /*
   * TODO: Handle function arguments by looking at assertType and setReturnType.
   */

  // TODO: handle PHI and select nodes

  return nullptr;
}

Type *TypeAnalysis::getMemOIRType(llvm::CallInst &call_inst) {
  /*
   * Look up the call instruction to see if we have a memoized Type.
   */
  auto found_summary = type_summaries.find(&call_inst);
  if (found_summary != type_summaries.end()) {
    return found_summary->second;
  }

  /*
   * If the call instruction is not memoized,
   *   then we need to create its Type
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
   * Build the Type for the given MemOIR type call.
   */
  Type *type_summary;
  switch (callee_enum) {
    case MemOIR_Func::UINT64_TYPE:
    case MemOIR_Func::UINT32_TYPE:
    case MemOIR_Func::UINT16_TYPE:
    case MemOIR_Func::UINT8_TYPE:
    case MemOIR_Func::INT64_TYPE:
    case MemOIR_Func::INT32_TYPE:
    case MemOIR_Func::INT16_TYPE:
    case MemOIR_Func::INT8_TYPE:
    case MemOIR_Func::FLOAT_TYPE:
    case MemOIR_Func::DOUBLE_TYPE:
    case MemOIR_Func::POINTER_TYPE:
      type_summary = &getPrimitiveType(callee_enum);
      break;
    case MemOIR_Func::REFERENCE_TYPE:
      type_summary = &getReferenceType(call_inst);
      break;
    case MemOIR_Func::STRUCT_TYPE:
      type_summary = &getStructType(call_inst);
      break;
    case MemOIR_Func::TENSOR_TYPE:
      type_summary = &getTensorType(call_inst);
      break;
    case MemOIR_Func::ASSOC_ARRAY_TYPE:
      type_summary = &getAssocArrayType(call_inst);
      break;
    case MemOIR_Func::SEQUENCE_TYPE:
      type_summary = &getSequenceType(call_inst);
      break;
    case MemOIR_Func::DEFINE_STRUCT_TYPE:
      type_summary = &defineStructType(call_inst);
      break;
    default:
      type_summary = nullptr;
      break;
  }

  /*
   * Memoize the Type we just built.
   */
  this->type_summaries[&call_inst] = type_summary;

  /*
   * Return the Type
   */
  return type_summary;
}

Type &TypeAnalysis::getPrimitiveType(MemOIR_Func function_enum) {
  switch (function_enum) {
    case UINT64_TYPE:
      return IntegerType::get(64, false);
    case UINT32_TYPE:
      return IntegerType::get(32, false);
    case UINT16_TYPE:
      return IntegerType::get(16, false);
    case UINT8_TYPE:
      return IntegerType::get(8, false);
    case INT64_TYPE:
      return IntegerType::get(64, true);
    case INT32_TYPE:
      return IntegerType::get(32, true);
    case INT16_TYPE:
      return IntegerType::get(16, true);
    case INT8_TYPE:
      return IntegerType::get(8, true);
    case FLOAT_TYPE:
      return FloatType::get();
    case DOUBLE_TYPE:
      return DoubleType::get();
    case POINTER_TYPE:
      return PointerType::get();
    default:
      MEMOIR_UNREACHABLE(
          "TypeAnalysis and FunctionNames have a mismatch in their type definitions");
  }
}

Type &TypeAnalysis::getIntegerType(llvm::CallInst &call_inst) {
  auto bitwidth_value = call_inst.getArgOperand(0);
  auto bitwidth_constant = dyn_cast<ConstantInt>(bitwidth_value);
  MEMOIR_NULL_CHECK(bitwidth_constant,
                    "bitwidth of integer type is not a constant int");

  auto is_signed_value = call_inst.getArgOperand(1);
  auto is_signed_constant = dyn_cast<ConstantInt>(is_signed_value);
  MEMOIR_NULL_CHECK(is_signed_constant,
                    "sign of integer type is not a constant int");

  auto bitwidth = bitwidth_constant->getZExtValue();
  auto is_signed = (is_signed_constant->getZExtValue() == 0) ? false : true;

  return IntegerType::get(bitwidth, is_signed);
}

Type &TypeAnalysis::getReferenceType(llvm::CallInst &call_inst) {
  auto referenced_type_value = call_inst.getArgOperand(0);
  auto referenced_type_call = dyn_cast<CallInst>(referenced_type_value);
  MEMOIR_NULL_CHECK(referenced_type_call."referenced type is not a call");

  auto referenced_type = getType(*referenced_type_call);
  MEMOIR_NULL_CHECK(referenced_type,
                    "referenced type does not have a type summary");

  return ReferenceType::get(*referenced_type);
}

Type &TypeAnalysis::getStructType(llvm::CallInst &call_inst) {
  auto name_value = call_inst.getArgOperand(0);

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(name_value);
  if (name_gep) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(name_value);
  }

  MEMOIR_NULL_CHECK(name_global, "struct type lookup is not a global variable");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "struct type lookup is not a constant array");

  auto name = name_constant->getAsCString();

  return StructType::get(name);
}

Type &TypeAnalysis::getTensorType(llvm::CallInst &call_inst) {
  auto element_value = call_inst.getArgOperand(0);
  auto element_call = dyn_cast<CallInst>(element_value);
  MEMOIR_NULL_CHECK(element_call,
                    "element type of tensor type is not a call instruction");

  auto element_type = this->getType(*element_call);
  MEMOIR_NULL_CHECK(element_type, "element type does not have a type summary");

  auto num_dimensions_value = call_inst.getArgOperand(1);
  MEMOIR_NULL_CHECK(num_dimensions_value, "number of dimensions is NULL");

  auto num_dimensions_constant = dyn_cast<ConstantInt>(num_dimensions_value);
  MEMOIR_NULL_CHECK(num_dimensions_constant,
                    "number of dimensions is not a constant");

  auto num_dimensions = num_dimensions_constant->getZExtValue();

  return TensorType::get(*element_type, num_dimensions);
}

Type &TypeAnalysis::getAssocArrayType(llvm::CallInst &call_inst) {
  auto key_value = call_inst.getArgOperand(0);
  auto key_call = dyn_cast<CallInst>(key_value);
  MEMOIR_NULL_CHECK(
      key_call,
      "key type of associative array type is not a call instruction");

  auto key_type = this->getType(*key_call);
  MEMOIR_NULL_CHECK(key_type, "key type does not have a type summary");

  auto value_value = call_inst.getArgOperand(0);
  auto value_call = dyn_cast<CallInst>(value_value);
  MEMOIR_NULL_CHECK(
      value_call,
      "value type of associative array type is not a call instruction");

  auto value_type = this->getType(*value_call);
  MEMOIR_NULL_CHECK(value_type, "value type does not have a type summary");

  return AssocArrayType::get(*key_type, *value_type);
}

Type &TypeAnalysis::getSequenceType(llvm::CallInst &call_inst) {
  auto element_value = call_inst.getArgOperand(0);
  auto element_call = dyn_cast<CallInst>(element_value);
  MEMOIR_NULL_CHECK(element_call,
                    "element type of tensor type is not a call instruction");

  auto element_type = this->getType(*element_call);
  MEMOIR_NULL_CHECK(element_type, "element type does not have a type summary");

  return SequenceType::get(*element_type);
}

Type &TypeAnalysis::defineStructType(llvm::CallInst &call_inst) {
  /*
   * Get the name of the Struct Type.
   */
  auto name_value = call_inst.getArgOperand(0);

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(name_value);
  if (name_gep) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(name_value);
  }
  MEMOIR_NULL_CHECK(name_global, "struct type lookup is not a global variable");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "struct type lookup is not a constant array");

  auto name = name_constant->getAsCString();

  /*
   * Get the number of fields of the Struct Type.
   *   NOTE: this number MUST be a constant
   */
  auto num_fields_value = call_inst.getArgOperand(1);
  auto num_fields_constant = dyn_cast<ConstantInt>(num_fields_value);
  MEMOIR_NULL_CHECK(num_fields_constant,
                    "number of fields is not a constant integer");

  auto num_fields = num_fields_constant->getZExtValue();

  /*
   * Determine the field types.
   */
  std::vector<Type *> field_type_summaries;
  for (auto field_index = 0; field_index < num_fields; field_index++) {
    auto arg_operand_index = field_index + 2;
    auto field_value = call_inst.getArgOperand(arg_operand_index);
    MEMOIR_NULL_CHECK(field_value, "field is NULL");

    auto field_type_summary = this->getType(*field_value);
    MEMOIR_NULL_CHECK(field_type_summary, "field does not have a type summary");

    field_type_summaries.push_back(field_type_summary);
  }

  /*
   * Build the Struct Type summary.
   */
  auto &type_summary = StructType::get(name, field_type_summaries);
  type_summaries[&call_inst] = &type_summary;

  return type_summary;
}

void TypeAnalysis::invalidate() {
  this->type_summaries.clear();

  return;
}

TypeAnalysis &TypeAnalysis::get(Module &M) {
  auto found_analysis = TypeAnalysis::analyses.find(&M);
  if (found_analysis != TypeAnalysis::analyses.end()) {
    return *(found_analysis->second);
  }

  auto new_analysis = new TypeAnalysis(M);
  TypeAnalysis::analyses[&M] = new_analysis;
  return *new_analysis;
}

void TypeAnalysis::invalidate(Module &M) {
  auto found_analysis = TypeAnalysis::analyses.find(&M);
  if (found_analysis != TypeAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis->second);
    analysis.invalidate();
  }

  return;
}

map<llvm::Module *, TypeAnalysis *> TypeAnalysis::analyses = {};

} // namespace llvm::memoir
