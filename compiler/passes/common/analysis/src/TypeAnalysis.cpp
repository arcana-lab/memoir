#include "common/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

TypeAnalysis::TypeAnalysis(Module &M) : M(M) {
  this->primitive_type_summaries = {
    { UINT64_TYPE, IntegerTypeSummary::get(64, false) },
    { UINT32_TYPE, IntegerTypeSummary::get(32, false) },
    { UINT16_TYPE, IntegerTypeSummary::get(16, false) },
    { UINT8_TYPE, IntegerTypeSummary::get(8, false) },
    { INT64_TYPE, IntegerTypeSummary::get(64, true) },
    { INT32_TYPE, IntegerTypeSummary::get(32, true) },
    { INT16_TYPE, IntegerTypeSummary::get(16, true) },
    { INT8_TYPE, IntegerTypeSummary::get(8, true) },
    { FLOAT_TYPE, FloatTypeSummary::get() },
    { DOUBLE_TYPE, DoubleTypeSummary::get() },
  };

  /*
   * Analyze the program.
   */
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto call_inst = dyn_cast<CallInst>(&I);
        if (!call_inst) {
          continue;
        }

        /*
         * Analyze the call.
         * The TypeSummary will be memoized
         */
        this->getTypeSummary(call_inst);
      }
    }
  }
}

TypeSummary *TypeAnalysis::getTypeSummary(llvm::CallInst &call_inst) {
  /*
   * Look up the call instruction to see if we have a memoized TypeSummary.
   */
  auto found_summary = type_summaries.find(&call_inst);
  if (found_summary != type_summaries.end()) {
    return found_summary.second;
  }

  /*
   * If the call instruction is not memoized,
   *   then we need to create its TypeSummary
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
   * Build the TypeSummary for the given MemOIR type call.
   */
  TypeSummary *type_summary;
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
      type_summary = getPrimitiveTypeSummary(callee_enum);
      break;
    case MemOIR_Func::INTEGER_TYPE:
      type_summary = getIntegerTypeSummary(call_inst);
      break;
    case MemOIR_Func::REFERENCE_TYPE:
      type_summary = getReferenceTypeSummary(call_inst);
      break;
    case MemOIR_Func::STRUCT_TYPE:
      type_summary = getStructTypeSummary(call_inst);
      break;
    case MemOIR_Func::TENSOR_TYPE:
      type_summary = getTensorTypeSummary(call_inst);
      break;
    case MemOIR_Func::DEFINE_STRUCT_TYPE:
      type_summary = defineStructTypeSummary(call_inst);
      break;
    case MemOIR_Func::NONE:
      type_summary = nullptr;
      break;
  }

  /*
   * Memoize the TypeSummary we just built.
   */
  auto type_summaries[&call_inst] = type_summary;

  /*
   * Return the TypeSummary
   */
  return type_summary;
}

TypeSummary *getPrimitiveTypeSummary(MemOIR_Func function_enum) {
  auto found_primitive = primitive_type_summaries.find(function_enum);
  if (found_primitive != primitive_type_summaries.end()) {
    auto primitive_type_summary = found_primitive.end();
    return primitive_type_summary;
  }

  assert(false && "in TypeSummary::getPrimitiveTypeSummary"
         && "TypeAnalysis and getTypeSummary have a mismatch");
}

TypeSummary *getIntegerTypeSummary(llvm::CallInst &call_inst) {
  auto bitwidth_value = call_inst.getArgOperand(0);
  auto bitwidth_constant = dyn_cast<ConstantInt>(bitwidth_value);
  assert(bitwidth_constant && "in TypeAnalysis::getIntegerTypeSummary"
         && "bitwidth of integer type is not a constant int");

  auto is_signed_value = call_inst.getArgOperand(1);
  auto is_signed_constant = dyn_cast<ConstantInt>(is_signed_value);
  assert(is_signed_constant && "in TypeAnalysis::getIntegerTypeSummary"
         && "sign of integer type is not a constant int");

  auto bitwidth = bitwidth_constant->getZExtValue();
  auto is_signed = (is_signed_constant->getZExtValue() == 0) ? false : true;

  return IntegerTypeSummary::get(bitwidth, is_signed);
}

TypeSummary *getReferenceTypeSummary(llvm::CallInst &call_inst) {
  auto referenced_type_value = call_inst.getArgOperand(0);
  auto referenced_type_call = dyn_cast<CallInst>(referenced_type_value);
  assert(referenced_type_call && "in TypeAnalysis::getTypeSummary"
         && "referenced type is not a call");

  auto referenced_type = getTypeSummary(referenced_type_call);
  assert(referenced_type && "in TypeAnalysis::getTypeSummary"
         && "referenced type does not have a type summary");

  return ReferenceTypeSummary::get(referenced_type);
}

TypeSummary *getStructTypeSummary(llvm::CallInst &call_inst) {
  auto name_value = call_inst.getArgOperand(0);

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(name_value);
  if (name_gep) {
    name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(name_value);
  }

  assert(name_global && "in TypeAnalysis::getTypeSummary"
         && "struct type lookup is not a global variable");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  assert(name_constant && "in TypeAnalysis::getTypeSummary"
         && "struct type lookup is not a constant array");

  auto name = name_constant->getAsCString();

  return StructTypeSummary::get(name);
}

TypeSummary *getTensorTypeSummary(llvm::CallInst &call_inst) {
  auto contained_value = call_inst.getArgOperand(0);
  auto contained_call = dyn_cast<CallInst>(referenced_value);
  assert(contained_call && "in TypeAnalysis::getTypeSummary"
         && "contained type of tensor type is not a call instruction");

  auto contained_type = getTypeSummary(contained_call);
  assert(contained_type && "in TypeAnalysis::getTypeSummary"
         && "contained type does not have a type summary");

  return TensorType::get(contained_type);
}

TypeSummary *defineStructTypeSummary(llvm::CallInst &call_inst) {
  /*
   * Get the name of the Struct Type.
   */
  auto name_value = call_inst.getArgOperand(0);

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(name_value);
  if (name_gep) {
    name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(name_value);
  }

  assert(name_global && "in TypeAnalysis::getTypeSummary"
         && "struct type lookup is not a global variable");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  assert(name_constant && "in TypeAnalysis::getTypeSummary"
         && "struct type lookup is not a constant array");

  auto name = name_constant->getAsCString();

  /*
   * Get the number of fields of the Struct Type.
   *   NOTE: this number MUST be a constant
   */
  auto num_fields_value = call_inst.getArgOperand(1);
  auto num_fields_constant = dyn_cast<ConstantInt>(num_fields_value);
  assert(num_fields_constant && "in TypeAnalysis::defineStructTypeSummary"
         && "number of fields is not a constant integer");

  auto num_fields = num_fields_constant.getZExtValue();

  /*
   * Determine the field types.
   */
  std::vector<TypeSummary *> field_type_summaries;
  for (auto field_index = 0; i < num_fields; i++) {
    auto arg_operand_index = field_index + 2;
    auto field_value = call_inst.getArgOperand(arg_operand_index);
    auto field_call = dyn_cast<CallInst>(field_value);
    assert(field_call && "in TypeAnalysis::defineStructTypeSummary"
           && "field is not a call instruction");

    auto field_type_summary = getTypeSummary(*field_call);
    assert(field_type_summary && "in TypeAnalysis::defineStructTypeSummary"
           && "field does not have a type summary");

    field_type_summaries.push_back(field_type_summary);
  }

  /*
   * Build the Struct Type summary.
   */
  auto type_summary = StructTypeSummary::get(name, field_types);
  type_summaries[&call_inst] = type_summary;

  return type_summary;
}

TypeAnalysis &TypeAnalysis::get(Module &M) {
  static TypeAnalysis singleton(M);

  return singleton;
}

} // namespace llvm::memoir
