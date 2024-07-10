#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// Top-level query.
Type *TypeChecker::type_of(MemOIRInst &I) {
  return TypeChecker::type_of(I.getCallInst());
}

Type *TypeChecker::type_of(llvm::Value &V) {
  TypeChecker TA;

  // Get the type of this value.
  auto *type = TA.analyze(V);

  // If the resulting type is NULL, return it.
  if (type == nullptr) {
    return nullptr;
  }

  // If the resuling type is a type variable, return NULL.
  if (isa<TypeVariable>(type)) {
    return nullptr;
  }

  return type;
}

// Analysis entry.
Type *TypeChecker::analyze(MemOIRInst &I) {
  return this->analyze(I.getCallInst());
}

Type *TypeChecker::analyze(llvm::Value &V) {
  // Handle each value case.
  if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return this->visit(*inst);
  } else if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return this->visitArgument(*arg);
  }

  return nullptr;
}

// LLVM Argument.
Type *TypeChecker::visitArgument(llvm::Argument &A) {
  // TODO: look for type assertions
  for (auto &use : A.uses()) {
    auto *user = use.getUser();
    // If the argument is used by an AssertTypeInst, we know its type.
    if (auto *assert = into<AssertTypeInst>(user)) {
      return this->analyze(assert->getTypeOperand());
    }

    // NOTE: this _could_ be extended to be recursive, but I will not add that
    // complexity until it is needed.
  }

  // Otherwise, we don't know the type of this argument!
  return nullptr;
}

// LLVM Instruction.
Type *TypeChecker::visitInstruction(llvm::Instruction &I) {
  return nullptr;
}

// TypeInsts.
Type *TypeChecker::visitUInt64TypeInst(UInt64TypeInst &I) {
  auto &type = Type::get_u64_type();

  return &type;
}

Type *TypeChecker::visitUInt32TypeInst(UInt32TypeInst &I) {
  auto &type = Type::get_u32_type();

  return &type;
}

Type *TypeChecker::visitUInt16TypeInst(UInt16TypeInst &I) {
  auto &type = Type::get_u16_type();

  return &type;
}

Type *TypeChecker::visitUInt8TypeInst(UInt8TypeInst &I) {
  auto &type = Type::get_u8_type();

  return &type;
}

Type *TypeChecker::visitUInt2TypeInst(UInt2TypeInst &I) {
  auto &type = Type::get_u2_type();

  return &type;
}

Type *TypeChecker::visitInt64TypeInst(Int64TypeInst &I) {
  auto &type = Type::get_i64_type();

  return &type;
}

Type *TypeChecker::visitInt32TypeInst(Int32TypeInst &I) {
  auto &type = Type::get_i32_type();

  return &type;
}

Type *TypeChecker::visitInt16TypeInst(Int16TypeInst &I) {
  auto &type = Type::get_i16_type();

  return &type;
}

Type *TypeChecker::visitInt8TypeInst(Int8TypeInst &I) {
  auto &type = Type::get_i8_type();

  return &type;
}

Type *TypeChecker::visitInt2TypeInst(Int2TypeInst &I) {
  auto &type = Type::get_i2_type();

  return &type;
}

Type *TypeChecker::visitBoolTypeInst(BoolTypeInst &I) {
  auto &type = Type::get_bool_type();

  return &type;
}

Type *TypeChecker::visitFloatTypeInst(FloatTypeInst &I) {
  auto &type = Type::get_f32_type();

  return &type;
}

Type *TypeChecker::visitDoubleTypeInst(DoubleTypeInst &I) {
  auto &type = Type::get_f64_type();

  return &type;
}

Type *TypeChecker::visitPointerTypeInst(PointerTypeInst &I) {
  auto &type = Type::get_ptr_type();

  return &type;
}

Type *TypeChecker::visitReferenceTypeInst(ReferenceTypeInst &I) {
  auto &referenced_type =
      MEMOIR_SANITIZE(this->analyze(I.getReferencedTypeOperand()),
                      "Could not determine referenced type!");

  auto &type = Type::get_ref_type(referenced_type);

  return &type;
}

Type *TypeChecker::visitDefineStructTypeInst(DefineStructTypeInst &I) {
  // Get the types of each field.
  vector<Type *> field_types;
  for (unsigned field_idx = 0; field_idx < I.getNumberOfFields(); field_idx++) {
    auto &field_type_value = I.getFieldTypeOperand(field_idx);
    auto *field_type = this->analyze(field_type_value);
    field_types.push_back(field_type);
  }

  // Build the StructType
  auto &type = StructType::define(I, I.getName(), field_types);

  return &type;
}

Type *TypeChecker::visitStructTypeInst(StructTypeInst &I) {
  // Get all users of the given name.
  auto &name_value = MEMOIR_SANITIZE(
      I.getNameOperand().stripPointerCasts(),
      "Could not get the name operand stripped of pointer casts");

  set<DefineStructTypeInst *> call_inst_users = {};
  for (auto *user : name_value.users()) {

    if (auto *type_def = into<DefineStructTypeInst>(user)) {
      call_inst_users.insert(type_def);
    } else if (auto *user_as_gep = dyn_cast<llvm::GetElementPtrInst>(user)) {
      for (auto *gep_user : user_as_gep->users()) {
        if (auto *type_def = into<DefineStructTypeInst>(gep_user)) {
          call_inst_users.insert(type_def);
        }
      }
    } else if (auto *user_as_gep = dyn_cast<llvm::ConstantExpr>(user)) {
      for (auto *gep_user : user_as_gep->users()) {
        if (auto *type_def = into<DefineStructTypeInst>(gep_user)) {
          call_inst_users.insert(type_def);
        }
      }
    }
  }

  // For each user, find the call to define the struct type.
  for (auto *call : call_inst_users) {
    auto *defined_type = this->visitDefineStructTypeInst(*call);
    MEMOIR_NULL_CHECK(defined_type,
                      "Could not determine the defined struct type");
    return defined_type;
  }

  MEMOIR_UNREACHABLE(
      "Could not find a definition for the given struct type name");
}

Type *TypeChecker::visitStaticTensorTypeInst(StaticTensorTypeInst &I) {
  // Get the length of dimensions.
  auto num_dimensions = I.getNumberOfDimensions();

  vector<size_t> length_of_dimensions;
  length_of_dimensions.resize(num_dimensions);

  for (unsigned dim_idx = 0; dim_idx < num_dimensions; dim_idx++) {
    length_of_dimensions[dim_idx] = I.getLengthOfDimension(dim_idx);
  }

  // Get the element type.
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementTypeOperand()),
                      "Could not determine element of StaticTensorType");

  // Build the new type.
  auto &type = Type::get_static_tensor_type(elem_type, length_of_dimensions);

  return &type;
}

Type *TypeChecker::visitTensorTypeInst(TensorTypeInst &I) {
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not determine element of TensorType");

  // Build the TensorType.
  auto &type = Type::get_tensor_type(elem_type, I.getNumberOfDimensions());

  return &type;
}

Type *TypeChecker::visitAssocArrayTypeInst(AssocArrayTypeInst &I) {
  auto &key_type = MEMOIR_SANITIZE(this->analyze(I.getKeyOperand()),
                                   "Could not determine key of AssocType");
  auto &value_type = MEMOIR_SANITIZE(this->analyze(I.getValueOperand()),
                                     "Could not determine value of AssocType");

  // Build the AssocArrayType.
  auto &type = AssocArrayType::get(key_type, value_type);

  return &type;
}

Type *TypeChecker::visitSequenceTypeInst(SequenceTypeInst &I) {
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not determine element of SequenceType");

  auto &type = SequenceType::get(elem_type);

  return &type;
}

// Allocation instructions.
Type *TypeChecker::visitStructAllocInst(StructAllocInst &I) {
  // Get the struct type.
  return this->analyze(I.getTypeOperand());
}

Type *TypeChecker::visitTensorAllocInst(TensorAllocInst &I) {

  // Determine the element type.
  auto &element_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not find element type of TensorAlloc!");

  // Build the TensorType.
  auto &type = TensorType::get(element_type, I.getNumberOfDimensions());

  return &type;
}

Type *TypeChecker::visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
  // Get the element types.
  auto &key_type = MEMOIR_SANITIZE(this->analyze(I.getKeyOperand()),
                                   "Could not find key type of AssocAlloc!");
  auto &value_type =
      MEMOIR_SANITIZE(this->analyze(I.getValueOperand()),
                      "Could not find value type of AssocAlloc!");

  // Create the sequence type.
  auto &assoc_type = AssocArrayType::get(key_type, value_type);

  return &assoc_type;
}

Type *TypeChecker::visitSequenceAllocInst(SequenceAllocInst &I) {
  // Get the element type.
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not determine SequenceAlloc element type!");

  // Create the sequence type.
  auto &seq_type = SequenceType::get(elem_type);

  return &seq_type;
}

// Reference Read Instructions.
Type *TypeChecker::visitReadInst(ReadInst &I) {
  // Get the collection type being accessed.
  auto *object_type = this->analyze(I.getObjectOperand());
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(object_type),
                      "ReadInst is accessing non-collection type!");

  // Return the element type, if it is a reference type.
  auto &element_type = collection_type.getElementType();

  // If the element type is a ReferenceType, unpack and return it.
  if (auto *ref_type = dyn_cast<ReferenceType>(&element_type)) {
    return &ref_type->getReferencedType();
  }

  // Otherwise, return NULL, as its an LLVM type.
  return nullptr;
}

Type *TypeChecker::visitStructReadInst(StructReadInst &I) {
  // Get the struct type.
  auto *object_type = this->analyze(I.getObjectOperand());
  auto &struct_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<StructType>(object_type),
                      "StructReadInst is accessing a non-collection type!");

  // Fetch the field information for the access.
  auto field_index = I.getFieldIndex();

  // Fetch the field type.
  auto &field_type = struct_type.getFieldType(field_index);

  // If the element type is a ReferenceType or a StructType, return it.
  if (isa<StructType>(&field_type)) {
    return &field_type;
  } else if (auto *ref_type = dyn_cast<ReferenceType>(&field_type)) {
    auto &referenced_type = ref_type->getReferencedType();
    return &referenced_type;
  }

  // Otherwise, it is an LLVM type, return NULL.
  return nullptr;
}

// Nested Access Instructions.
Type *TypeChecker::visitGetInst(GetInst &I) {
  // Get the type of collection being accessed.
  auto *object_type = this->analyze(I.getObjectOperand());
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(object_type),
                      "GetInst accessing non-collection type!");

  // Return the element type.
  auto &element_type = collection_type.getElementType();
  return &element_type;
}

Type *TypeChecker::visitStructGetInst(StructGetInst &I) {
  // Get the struct type.
  auto *object_type = this->analyze(I.getObjectOperand());
  auto &struct_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<StructType>(object_type),
                      "StructGetInst is accessing a non-collection type!");

  // Fetch the field information for the access.
  auto field_index = I.getFieldIndex();

  // Fetch the field type.
  auto &field_type = struct_type.getFieldType(field_index);

  // If the element type is a ReferenceType or a StructType, return it.
  if (isa<StructType>(&field_type)) {
    return &field_type;
  } else if (auto *ref_type = dyn_cast<ReferenceType>(&field_type)) {
    auto &referenced_type = ref_type->getReferencedType();
    return &referenced_type;
  }

  // Otherwise, it is an LLVM type, return NULL.
  return nullptr;
}

// Write access instructions.
Type *TypeChecker::visitWriteInst(WriteInst &I) {
  return this->analyze(I.getObjectOperand());
}

// SSA Instructions
Type *TypeChecker::visitUsePHIInst(UsePHIInst &I) {
  return this->analyze(I.getUsedCollection());
}

Type *TypeChecker::visitDefPHIInst(DefPHIInst &I) {
  return this->analyze(I.getDefinedCollection());
}

Type *TypeChecker::visitArgPHIInst(ArgPHIInst &I) {
  return this->analyze(I.getInputCollection());
}

Type *TypeChecker::visitRetPHIInst(RetPHIInst &I) {
  return this->analyze(I.getInputCollection());
}

// SSA collection operations.
Type *TypeChecker::visitCopyInst(CopyInst &I) {
  return this->analyze(I.getCopiedCollection());
}

Type *TypeChecker::visitInsertInst(InsertInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeChecker::visitRemoveInst(RemoveInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeChecker::visitSwapInst(SwapInst &I) {
  return this->analyze(I.getFromCollection());
}

Type *TypeChecker::visitFoldInst(FoldInst &I) {
  return this->analyze(I.getInitial());
}

// SSA assoc operations.
Type *TypeChecker::visitAssocInsertInst(AssocInsertInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeChecker::visitAssocRemoveInst(AssocRemoveInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeChecker::visitAssocHasInst(AssocHasInst &I) {
  // We _could_ use this opportunity to unify with an abstract assoc, but we
  // won't unless deemed necessary.
  return nullptr;
}

Type *TypeChecker::visitAssocKeysInst(AssocKeysInst &I) {

  // Get the incoming associative array type.
  auto *input_type = this->analyze(I.getCollection());

  // Cast it to an assoc type, if it is not one, error!
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(input_type),
                      "AssocKeys used on non-assoc operand.");

  // Fetch the key type.
  auto &key_type = assoc_type.getKeyType();

  // Construct a sequence type for it.
  auto &seq_type = SequenceType::get(key_type);

  return &seq_type;
}

// LLVM Instructions.
Type *TypeChecker::visitExtractValueInst(llvm::ExtractValueInst &I) {
  // Get the aggregate type.
  auto &aggregate =
      MEMOIR_SANITIZE(I.getAggregateOperand(),
                      "Aggregate operand of ExtractValueInst is NULL!");

  // We will simply say that aggregate types _always_ hold the same type as its
  // operands. This is a fine assumption becuase, so far, only seq swap produces
  // an aggregate.
  return this->analyze(aggregate);
}

Type *TypeChecker::visitLoadInst(llvm::LoadInst &I) {
  // If we have load instruction, trace back to its global variable and find the
  // original store to it.
  auto *load_ptr = I.getPointerOperand();
  auto *global = dyn_cast<llvm::GlobalVariable>(load_ptr->stripPointerCasts());

  // If the load is not from a GlobalVariable, return NULL.
  if (!global) {
    return nullptr;
  }

  // Find the original store for this global variable.
  for (auto *user : global->users()) {
    if (auto *store_inst = dyn_cast<llvm::StoreInst>(user)) {
      auto *store_value = store_inst->getValueOperand();

      auto *stored_type = this->analyze(*store_value);

      if (stored_type != nullptr) {
        return stored_type;
      }
    }
  }

  // Otherwise, return NULL.
  return nullptr;
}

Type *TypeChecker::visitPHINode(llvm::PHINode &I) {

  // Check that we have not already visited this PHI.
  auto found = value_bindings.find(&I);
  if (found != value_bindings.end()) {
    // If we have, return its temporary binding.
    return found->second;
  }

  // Create a type variable for the PHI node.
  auto &phi_type = this->new_type_variable();
  this->value_bindings[&I] = &phi_type;

  // For each incoming value, visit it, and unify it with the PHI type.
  for (auto &incoming : I.incoming_values()) {
    auto &incoming_value =
        MEMOIR_SANITIZE(incoming.get(), "Found a NULL value when typing!");
    auto *incoming_type = this->analyze(incoming_value);

    // Unify the PHI with the incoming type.
    this->unify(&phi_type, incoming_type);
  }

  // Find the resulting type of the PHI and return it.
  return this->find(&phi_type);
}

Type *TypeChecker::visitLLVMCallInst(llvm::CallInst &I) {

  // Fetch the called function.
  auto *called_function = I.getCalledFunction();
  if (called_function == nullptr || called_function->empty()) {
    // TODO: this can be made less conservative.
    return nullptr;
  }

  // If the returned type is not a ptr, return NULL.
  if (not isa<llvm::PointerType>(called_function->getReturnType())) {
    return nullptr;
  }

  // Check if there is a ReturnTypeInst in the entry basic block.
  for (auto &I : called_function->getEntryBlock()) {
    if (auto return_type_inst = into<ReturnTypeInst>(&I)) {
      return this->analyze(return_type_inst->getTypeOperand());
    }
  }

  // Otherwise, try to type the returned values.
  for (auto &BB : *called_function) {
    auto *terminator = BB.getTerminator();
    if (auto *return_inst = dyn_cast_or_null<llvm::ReturnInst>(terminator)) {
      auto *return_value = return_inst->getReturnValue();

      return this->analyze(*return_value);
    }
  }

  // If we've gotten this far and haven't found anythin, return NULL.
  return nullptr;
}

// Union-find
TypeVariable &TypeChecker::new_type_variable() {
  auto *typevar = new TypeVariable(this->current_id++);

  this->type_bindings[typevar] = typevar;

  return *typevar;
}

Type *TypeChecker::find(Type *t) {
  // If t is not a type variable, return it.
  if (not isa_and_nonnull<TypeVariable>(t)) {
    return t;
  }

  // Otherwise, let's find the binding for the type variable.
  auto *typevar = cast<TypeVariable>(t);

  // Lookup the type binding.
  auto found = this->type_bindings.find(typevar);

  // If the type has no binding, insert and return itself.
  if (found == this->type_bindings.end()) {
    this->type_bindings[typevar] = typevar;
    return typevar;
  }

  // If the parent is itself, return it.
  const auto &parent = found->second;
  if (parent == typevar) {
    return typevar;
  }

  // Otherwise, recurse on the parent.
  const auto &new_parent = this->find(parent);

  // Update the type variable's binding.
  this->type_bindings[typevar] = new_parent;

  // Return the new parent.
  return new_parent;
}

Type *TypeChecker::unify(Type *t, Type *u) {
  // Find each type's equivalence class.
  t = this->find(t);
  u = this->find(u);

  // If they are derived types, recurse on their constituents.
  // NOTE: this is not currently needed in our scheme. If it becomes necessary
  // it should be implemented here.

  // Merge the two types.

  // If they are equal, they are already merged!
  if (t == u) {
    return t;
  }

  if (auto *tvar = dyn_cast<TypeVariable>(t)) {
    this->type_bindings[tvar] = u;
    return u;
  }

  if (auto *uvar = dyn_cast<TypeVariable>(u)) {
    this->type_bindings[uvar] = t;
    return t;
  }

  // If neither t nor u is a type variable, and they are not equal, we have a
  // type error!
  MEMOIR_UNREACHABLE("Unable to merge types!");
}

// Constructor
TypeChecker::TypeChecker() : current_id(0) {}

// Destructor
TypeChecker::~TypeChecker() {
  for (const auto &[typevar, type] : type_bindings) {
    delete typevar;
  }
}

} // namespace llvm::memoir
