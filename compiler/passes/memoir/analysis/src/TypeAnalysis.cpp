#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

TypeAnalysis::TypeAnalysis() {
  // Do nothing.
}

RetTy *TypeAnalysis::getType(llvm::Value &V) {
  /*
   * Trace back the value to find the associated
   *   Type, if it exists.
   */

  /*
   * If we have an instruction, visit it.
   */
  if (auto inst = dyn_cast<llvm::Instruction>(&V)) {
    return this->visit(*inst);
  }

  /*
   * TODO: Handle function arguments by looking at assertType.
   */

  /*
   * TODO: Handle function returns by looking at setReturnType.
   */

  return nullptr;
}

/*
 * TypeInsts
 */
RetTy TypeAnalysis::visitIntegerTypeInst(IntegerTypeInst &I) {
  switch (I.getBitwidth()) {
    case 64: {
      return (I.isSigned()) ? Type::get_i64_type() : Type::get_u64_type();
    }
    case 32: {
      return (I.isSigned()) ? Type::get_i32_type() : Type::get_u32_type();
    }
    case 16: {
      return (I.isSigned()) ? Type::get_i16_type() : Type::get_u16_type();
    }
    case 8: {
      return (I.isSigned()) ? Type::get_i8_type() : Type::get_u8_type();
    }
    case 1: {
      MEMOIR_ASSERT(I.isSigned(), "Unsigned 1 bit integer is undefined");
      return Type::get_i1_type();
    }
    default: {
      MEMOIR_UNREACHABLE("Unknown integer type");
    }
  }
}

RetTy TypeAnalysis::visitFloatTypeInst(FloatTypeInst &I) {
  return &(Type::get_f32_type());
}

RetTy TypeAnalysis::visitDoubleTypeInst(DoubleTypeInst &I) {
  return &(Type::get_f64_type());
}

RetTy TypeAnalysis::visitPointerTypeInst(PointerTypeInst &I) {
  return &(Type::get_ptr_type());
}

RetTy TypeAnalysis::visitReferenceTypeInst(ReferenceTypeInst &I) {
  auto referenced_type = I.getReferencedType();
  MEMOIR_NULL_CHECK(referenced_type,
                    "Referenced type is an unknown LLVM Value");

  return &(Type::get_ref_type(*referenced_type));
}

RetTy TypeAnalysis::visitDefineStructTypeInst(DefineStructTypeInst &I) {
  /*
   * See if an existing type exisits.
   */
  if (auto found = this->find_existing(I)) {
    return found;
  }

  /*
   * Get the types of each field.
   */
  vector<Type *> field_types;
  for (auto field_idx = 0; field_idx < I.getNumberOfFields(); field_idx++) {
    field_types.push_back(I.getFieldType(field_idx));
  }

  /*
   * Build the StructType
   */
  auto &type = StructType::define(I.getCallInst(), I.getName(), field_types);

  /*
   * Memoize and return
   */
  this->memoize(I, type);
  return &type;
}

RetTy TypeAnalysis::visitStructTypeInst(StructTypeInst &I) {
  /*
   * See if an existing type exisits.
   */
  if (auto found = this->find_existing(I)) {
    return found;
  }

  /*
   * Get all users of the given name.
   */
  auto &name_value = I.getNameOperand();

  set<llvm::CallInst *> call_inst_users = {};
  for (auto *user : name_value.users()) {
    if (auto *user_as_call = dyn_cast<llvm::CallInst>(user)) {
      call_inst_users.insert(user_as_call);
    } else if (auto *user_as_gep = dyn_cast<llvm::GetElementPtrInst>(user)) {
      for (auto *gep_user : user_as_gep.users()) {
        if (auto *gep_user_as_call = dyn_cast<llvm::CallInst>(gep_user)) {
          call_inst_users.insert(gep_user_as_call);
        }
      }
    }
  }

  /*
   * For each user, find the call to define the struct type.
   */
  for (auto *call : call_inst_users) {
    if (FunctionNames::is_memoir_call(*call)) {
      auto func_enum = FunctionNames::get_memoir_enum(*call);

      if (func_enum == MemOIR_Func::DEFINE_STRUCT_TYPE) {
        auto defined_type = this->getType(call);
        this->memoize(I, defined_type);
        return defined_type;
      }
    }
  }

  MEMOIR_UNREACHABLE(
      "Could not find a definition for the given struct type name");
}

RetTy TypeAnalysis::visitStaticTensorTypeInst(StaticTensorTypeInst &I) {
  /*
   * See if an existing type exisits.
   */
  if (auto found = this->find_existing(I)) {
    return found;
  }

  /*
   * Get the length of dimensions
   */
  vector<size_t> length_of_dimensions;
  for (auto dim_idx = 0; dim_idx < I.getNumberOfDimensions(); dim_idx++) {
    length_of_dimensions.push_back(I.getLengthOfDimension(dim_idx));
  }

  /*
   * Build the new type.
   */
  auto &type = StaticTensorType::get(I.getElementType(),
                                     I.getNumberOfDimensions(),
                                     length_of_dimensions);

  /*
   * Memoize and return.
   */
  this->memoize(I, type);
  return &type;
}

RetTy TypeAnalysis::visitTensorTypeInst(TensorTypeInst &I) {
  /*
   * See if an existing type exisits.
   */
  if (auto found = this->find_existing(I)) {
    return found;
  }

  /*
   * Build the TensorType.
   */
  auto &type = TensorType::get(I.getElementType(), I.getNumberOfDimensions());

  /*
   * Memoize and return.
   */
  this->memoize(I, type);
  return &type;
}

RetTy TypeAnalysis::visitAssocArrayTypeInst(AssocArrayTypeInst &I) {
  /*
   * See if an existing type exisits.
   */
  if (auto found = this->find_existing(I)) {
    return found;
  }

  /*
   * Build the AssocArrayType.
   */
  auto &type = AssocArrayType::get(I.getKeyType(), I.getValueType());

  /*
   * Memoize and return.
   */
  this->memoize(I, type);
  return &type;
}

RetTy TypeAnalysis::visitSequenceTypeInst(SequenceTypeInst &I) {
  /*
   * See if an existing type exisits.
   */
  if (auto found = this->find_existing(I)) {
    return found;
  }

  /*
   * Build the SequenceType.
   */
  auto &type = SequenceType::get(I.getElementType());

  /*
   * Memoize and return.
   */
  this->memoize(I, type);
  return &type;
}

/*
 * AllocInsts
 */

/*
 * LLVM Insts
 */
RetTy TypeAnalysis::visitLoadInst(llvm::LoadInst &load_inst) {
  /*
   * See if an existing type exisits.
   */
  if (auto found = this->find_existing(I)) {
    return found;
  }

  /*
   * If we have load instruction, trace back to its
   *   global variable and find the original store to it.
   */
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

      return this->getType(store_value);
    }
  }
}

/*
 * Internal helper functions
 */
RetTy TypeAnalysis::findExisting(llvm::Value &V) {
  auto found_type = this->value_to_type.find(&V);
  if (found_type != this->value_to_type.end()) {
    return found_type->second;
  }

  return nullptr;
}

RetTy TypeAnalysis::findExisting(MemOIRInst &I) {
  return this->findExisting(I.getCallInst());
}

void TypeAnalysis::memoize(llvm::Value &V, Type &T) {
  this->value_to_type[&V] = &T;
}

void TypeAnalysis::memoize(MemOIRInst &I, Type &T) {
  this->memoize(I.getCallInst(), T);
}

/*
 * Singleton access
 */
void TypeAnalysis::invalidate() {
  this->type_summaries.clear();

  return;
}

TypeAnalysis &TypeAnalysis::get() {
  static TypeAnalysis TA;
  return TA;
}

void TypeAnalysis::invalidate(Module &M) {
  auto found_analysis = TypeAnalysis::analyses.find(&M);
  if (found_analysis != TypeAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis->second);
    analysis.invalidate();
  }

  return;
}

} // namespace llvm::memoir
