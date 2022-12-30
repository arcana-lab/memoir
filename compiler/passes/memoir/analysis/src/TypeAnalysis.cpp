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
  if (auto arg = dyn_cast<llvm::Argument>(&V)) {
    MEMOIR_UNREACHABLE("Handling for arguments is currently unsupported");
  }

  return nullptr;
}

/*
 * Helper macros
 */
#define CHECK_MEMOIZED(I)                                                      \
  /* See if an existing type exists, if it does, early return. */              \
  if (auto found = this->find_existing(I)) {                                   \
    return found;                                                              \
  }

#define MEMOIZE_AND_RETURN(I, T)                                               \
  /* Memoize the type */                                                       \
  this->memoize(I, type);                                                      \
  /* Return */                                                                 \
  return &type

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
  CHECK_MEMOIZED(I);

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

  MEMOIZE_AND_RETURN(I, type);
}

RetTy TypeAnalysis::visitStructTypeInst(StructTypeInst &I) {
  CHECK_MEMOIZED(I);

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
        MEMOIR_NULL_CHECK(defined_type,
                          "Could not determine the defined struct type");
        MEMOIZE_AND_RETURN(I, *defined_type);
      }
    }
  }

  MEMOIR_UNREACHABLE(
      "Could not find a definition for the given struct type name");
}

RetTy TypeAnalysis::visitStaticTensorTypeInst(StaticTensorTypeInst &I) {
  CHECK_MEMOIZED(I);

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

  MEMOIZE_AND_RETURN(I, type);
}

RetTy TypeAnalysis::visitTensorTypeInst(TensorTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Build the TensorType.
   */
  auto &type = TensorType::get(I.getElementType(), I.getNumberOfDimensions());

  MEMOIZE_AND_RETURN(I, type);
}

RetTy TypeAnalysis::visitAssocArrayTypeInst(AssocArrayTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Build the AssocArrayType.
   */
  auto &type = AssocArrayType::get(I.getKeyType(), I.getValueType());

  MEMOIZE_AND_RETURN(I, type);
}

RetTy TypeAnalysis::visitSequenceTypeInst(SequenceTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Build the SequenceType.
   */
  auto &type = SequenceType::get(I.getElementType());

  MEMOIZE_AND_RETURN(I, type);
}

/*
 * AllocInsts
 */
RetTy TypeAnalysis::visitStructAllocInst(StructAllocInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Recurse on the type operand.
   */
  auto type = this->getType(I.getTypeOperand());

  MEMOIR_NULL_CHECK(type,
                    "Could not determine the struct type being allocated");

  MEMOIZE_AND_RETURN(I, *type);
}

RetTy TypeAnalysis::TensorAllocInst(TensorAllocInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the element type.
   */
  auto element_type = this->getType(I.getElementOperand());
  MEMOIR_NULL_CHECK(element_type, "Element type of tensor allocation is NULL");

  /*
   * Build the TensorType.
   */
  auto &type = Type::get_tensor_type(*element_type, I.getNumberOfDimensions());

  MEMOIZE_AND_RETURN(I, type);
}

RetTy TypeAnalysis::AssocArrayAllocInst(TensorAllocInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the Key and Value types.
   */
  auto key_type = this->getType(I.getKeyOperand());
  MEMOIR_NULL_CHECK(key_type, "Key type of assoc array allocation is NULL");
  auto value_type = this->getType(I.getValueOperand());
  MEMOIR_NULL_CHECK(value_type, "Value type of assoc array allocation is NULL");

  /*
   * Build the AssocArrayType.
   */
  auto &type = Type::get_assoc_array_type(*key_type, *value_type);

  MEMOIZE_AND_RETURN(I, type);
}

RetTy TypeAnalysis::SequenceAllocInst(TensorAllocInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the element type.
   */
  auto element_type = this->getType(I.getElementOperand());
  MEMOIR_NULL_CHECK(element_type, "Element type of sequence is NULL");

  MEMOIZE_AND_RETURN(I, type);
}

/*
 * Type Checking Instructions
 */
RetTy TypeAnalysis::visitAssertStructTypeInst(AssertStructTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, *type);
}

RetTy TypeAnalysis::visitAssertCollectionTypeInst(AssertCollectionTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, *type);
}

RetTy TypeAnalysis::visitReturnTypeInst(ReturnTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, *type);
}

/*
 * LLVM Instructions
 */
RetTy TypeAnalysis::visitLoadInst(llvm::LoadInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * If we have load instruction, trace back to its
   *   global variable and find the original store to it.
   */
  auto load_ptr = I.getPointerOperand();

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

      auto stored_type = this->getType(store_value);
      MEMOIR_NULL_CHECK(stored_type,
                        "Could not determine the type being loaded");

      MEMOIZE_AND_RETURN(I, stored_type);
    }
  }
}

RetTy TypeAnalysis::visitLLVMCallInst(llvm::CallInst &I) {
  CHECK_MEMOIZED(I);

  MEMOIR_UNREACHABLE(
      "Handling for interprocedural type analysis is not implemented");

  //  MEMOIZE_AND_RETURN(I, type);
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
void TypeAnalysis::_invalidate() {
  this->value_to_type.clear();
  return;
}

TypeAnalysis &TypeAnalysis::get() {
  static TypeAnalysis TA;
  return TA;
}

void TypeAnalysis::invalidate() {
  TypeAnalysis::get()._invalidate();
  return;
}

} // namespace llvm::memoir
