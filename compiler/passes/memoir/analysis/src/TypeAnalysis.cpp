#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

/*
 * Initialization
 */
TypeAnalysis::TypeAnalysis() {
  // Do nothing.
}

/*
 * Queries
 */
Type *TypeAnalysis::analyze(llvm::Value &V) {
  return TypeAnalysis::get().getType(V);
}

/*
 * Analysis
 */
Type *TypeAnalysis::getType(llvm::Value &V) {
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
#define CHECK_MEMOIZED(V)                                                      \
  /* See if an existing type exists, if it does, early return. */              \
  if (auto found = this->findExisting(V)) {                                    \
    return found;                                                              \
  }

#define MEMOIZE_AND_RETURN(V, T)                                               \
  /* Memoize the type */                                                       \
  this->memoize(V, T);                                                         \
  /* Return */                                                                 \
  return T

/*
 * Base case
 */
Type *TypeAnalysis::visitInstruction(llvm::Instruction &I) {
  return nullptr;
}

/*
 * TypeInsts
 */
Type *TypeAnalysis::visitUInt64TypeInst(UInt64TypeInst &I) {
  return &(Type::get_u64_type());
}

Type *TypeAnalysis::visitUInt32TypeInst(UInt32TypeInst &I) {
  return &(Type::get_u32_type());
}

Type *TypeAnalysis::visitUInt16TypeInst(UInt16TypeInst &I) {
  return &(Type::get_u16_type());
}

Type *TypeAnalysis::visitUInt8TypeInst(UInt8TypeInst &I) {
  return &(Type::get_u8_type());
}

Type *TypeAnalysis::visitInt64TypeInst(Int64TypeInst &I) {
  return &(Type::get_i64_type());
}

Type *TypeAnalysis::visitInt32TypeInst(Int32TypeInst &I) {
  return &(Type::get_i32_type());
}

Type *TypeAnalysis::visitInt16TypeInst(Int16TypeInst &I) {
  return &(Type::get_i16_type());
}

Type *TypeAnalysis::visitInt8TypeInst(Int8TypeInst &I) {
  return &(Type::get_i8_type());
}

Type *TypeAnalysis::visitBoolTypeInst(BoolTypeInst &I) {
  return &(Type::get_i1_type());
}

Type *TypeAnalysis::visitFloatTypeInst(FloatTypeInst &I) {
  return &(Type::get_f32_type());
}

Type *TypeAnalysis::visitDoubleTypeInst(DoubleTypeInst &I) {
  return &(Type::get_f64_type());
}

Type *TypeAnalysis::visitPointerTypeInst(PointerTypeInst &I) {
  return &(Type::get_ptr_type());
}

Type *TypeAnalysis::visitReferenceTypeInst(ReferenceTypeInst &I) {
  auto &referenced_type = I.getReferencedType();

  return &(Type::get_ref_type(referenced_type));
}

Type *TypeAnalysis::visitDefineStructTypeInst(DefineStructTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Get the types of each field.
   */
  vector<Type *> field_types;
  for (auto field_idx = 0; field_idx < I.getNumberOfFields(); field_idx++) {
    field_types.push_back(&(I.getFieldType(field_idx)));
  }

  /*
   * Build the StructType
   */
  auto &type = StructType::define(I, I.getName(), field_types);

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitStructTypeInst(StructTypeInst &I) {
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
      for (auto *gep_user : user_as_gep->users()) {
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
        auto defined_type = this->getType(*call);
        MEMOIR_NULL_CHECK(defined_type,
                          "Could not determine the defined struct type");
        MEMOIZE_AND_RETURN(I, defined_type);
      }
    }
  }

  MEMOIR_UNREACHABLE(
      "Could not find a definition for the given struct type name");
}

Type *TypeAnalysis::visitStaticTensorTypeInst(StaticTensorTypeInst &I) {
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
  auto &type =
      Type::get_static_tensor_type(I.getElementType(), length_of_dimensions);

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitTensorTypeInst(TensorTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Build the TensorType.
   */
  auto &type =
      Type::get_tensor_type(I.getElementType(), I.getNumberOfDimensions());

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitAssocArrayTypeInst(AssocArrayTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Build the AssocArrayType.
   */
  auto &type = AssocArrayType::get(I.getKeyType(), I.getValueType());

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitSequenceTypeInst(SequenceTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Build the SequenceType.
   */
  auto &type = SequenceType::get(I.getElementType());

  MEMOIZE_AND_RETURN(I, &type);
}

/*
 * AllocInsts
 */
Type *TypeAnalysis::visitStructAllocInst(StructAllocInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Recurse on the type operand.
   */
  auto type = this->getType(I.getTypeOperand());

  MEMOIR_NULL_CHECK(type,
                    "Could not determine the struct type being allocated");

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitTensorAllocInst(TensorAllocInst &I) {
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

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
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

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitSequenceAllocInst(SequenceAllocInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the element type.
   */
  auto element_type = this->getType(I.getElementOperand());
  MEMOIR_NULL_CHECK(element_type, "Element type of sequence is NULL");

  /*
   * Build the SequenceType.
   */
  auto &type = Type::get_sequence_type(*element_type);

  MEMOIZE_AND_RETURN(I, &type);
}

/*
 * Type Checking Instructions
 */
Type *TypeAnalysis::visitAssertStructTypeInst(AssertStructTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitAssertCollectionTypeInst(AssertCollectionTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitReturnTypeInst(ReturnTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, type);
}

/*
 * LLVM Instructions
 */
Type *TypeAnalysis::visitLLVMCallInst(llvm::CallInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * TODO
   * Implement interprocedural analysis.
   */
  auto type = nullptr;

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitLoadInst(llvm::LoadInst &I) {
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
      errs() << "Found store inst " << *store_inst << "\n";

      auto store_value = store_inst->getValueOperand();

      auto stored_type = this->getType(*store_value);
      MEMOIR_NULL_CHECK(stored_type,
                        "Could not determine the type being loaded");

      MEMOIZE_AND_RETURN(I, stored_type);
    }
  }

  /*
   * Otherwise, return NULL.
   */
  return nullptr;
}

Type *TypeAnalysis::visitPHINode(llvm::PHINode &I) {
  CHECK_MEMOIZED(I);

  /*
   * Get all incoming types.
   */
  set<Type *> incoming_types;
  for (auto &incoming : I.incoming_values()) {
    /*
     * Get the incoming value.
     */
    MEMOIR_NULL_CHECK(incoming, "Incoming value to PHI node is NULL.");
    auto &incoming_value = *(incoming.get());

    /*
     * Get the Type of the incoming value.
     */
    auto incoming_type = this->getType(incoming_value);

    /*
     * Insert the incoming Type into the set of incoming types.
     * NOTE: there should only be ONE type in the set after this loop.
     */
    incoming_types.insert(incoming_type);
  }

  /*
   * Check that all incoming types are the same.
   */
  MEMOIR_ASSERT((incoming_types.size() == 1),
                "Could not statically determine the type of PHI node.");

  /*
   * Get the single Type.
   */
  auto type = *(incoming_types.begin());

  MEMOIZE_AND_RETURN(I, type);
}

/*
 * Internal helper functions
 */
Type *TypeAnalysis::findExisting(llvm::Value &V) {
  auto found_type = this->value_to_type.find(&V);
  if (found_type != this->value_to_type.end()) {
    return found_type->second;
  }

  return nullptr;
}

Type *TypeAnalysis::findExisting(MemOIRInst &I) {
  return this->findExisting(I.getCallInst());
}

void TypeAnalysis::memoize(llvm::Value &V, Type *T) {
  this->value_to_type[&V] = T;
}

void TypeAnalysis::memoize(MemOIRInst &I, Type *T) {
  this->memoize(I.getCallInst(), T);
}

/*
 * Management
 */
void TypeAnalysis::_invalidate() {
  this->value_to_type.clear();
  return;
}

TypeAnalysis *TypeAnalysis::TA = nullptr;

TypeAnalysis &TypeAnalysis::get() {
  if (TypeAnalysis::TA == nullptr) {
    TypeAnalysis::TA = new TypeAnalysis();
  }
  return *(TypeAnalysis::TA);
}

void TypeAnalysis::invalidate() {
  TypeAnalysis::get()._invalidate();
  return;
}

} // namespace llvm::memoir
