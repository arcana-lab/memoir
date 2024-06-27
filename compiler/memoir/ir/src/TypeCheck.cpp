#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// Top-level query.
Type *TypeAnalysis::type_of(MemOIRInst &I) {
  return TypeAnalysis::type_of(I.getCallInst());
}

Type *TypeAnalysis::type_of(llvm::Value &V) {
  TypeAnalysis TA;

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
Type *TypeAnalysis::analyze(MemOIRInst &I) {
  return this->analyze(I.getCallInst());
}

Type *TypeAnalysis::analyze(llvm::Value &V) {
  // Handle each value case.
  if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return this->visit(*inst);
  } else if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return this->visitArgument(*arg);
  }

  return nullptr;
}

// LLVM Argument.
Type *TypeAnalysis::visitArgument(llvm::Argument &A) {
  // TODO: look for type assertions
  return nullptr;
}

// LLVM Instruction.
Type *TypeAnalysis::visitInstruction(llvm::Instruction &I) {
  return nullptr;
}

// TypeInsts.
Type *TypeAnalysis::visitUInt64TypeInst(UInt64TypeInst &I) {
  auto &type = Type::get_u64_type();

  return &type;
}

Type *TypeAnalysis::visitUInt32TypeInst(UInt32TypeInst &I) {
  auto &type = Type::get_u32_type();

  return &type;
}

Type *TypeAnalysis::visitUInt16TypeInst(UInt16TypeInst &I) {
  auto &type = Type::get_u16_type();

  return &type;
}

Type *TypeAnalysis::visitUInt8TypeInst(UInt8TypeInst &I) {
  auto &type = Type::get_u8_type();

  return &type;
}

Type *TypeAnalysis::visitUInt2TypeInst(UInt2TypeInst &I) {
  auto &type = Type::get_u2_type();

  return &type;
}

Type *TypeAnalysis::visitInt64TypeInst(Int64TypeInst &I) {
  auto &type = Type::get_i64_type();

  return &type;
}

Type *TypeAnalysis::visitInt32TypeInst(Int32TypeInst &I) {
  auto &type = Type::get_i32_type();

  return &type;
}

Type *TypeAnalysis::visitInt16TypeInst(Int16TypeInst &I) {
  auto &type = Type::get_i16_type();

  return &type;
}

Type *TypeAnalysis::visitInt8TypeInst(Int8TypeInst &I) {
  auto &type = Type::get_i8_type();

  return &type;
}

Type *TypeAnalysis::visitInt2TypeInst(Int2TypeInst &I) {
  auto &type = Type::get_i2_type();

  return &type;
}

Type *TypeAnalysis::visitBoolTypeInst(BoolTypeInst &I) {
  auto &type = Type::get_bool_type();

  return &type;
}

Type *TypeAnalysis::visitFloatTypeInst(FloatTypeInst &I) {
  auto &type = Type::get_f32_type();

  return &type;
}

Type *TypeAnalysis::visitDoubleTypeInst(DoubleTypeInst &I) {
  auto &type = Type::get_f64_type();

  return &type;
}

Type *TypeAnalysis::visitPointerTypeInst(PointerTypeInst &I) {
  auto &type = Type::get_ptr_type();

  return &type;
}

Type *TypeAnalysis::visitReferenceTypeInst(ReferenceTypeInst &I) {
  auto &referenced_type =
      MEMOIR_SANITIZE(this->analyze(I.getReferencedTypeOperand()),
                      "Could not determine referenced type!");

  auto &type = Type::get_ref_type(referenced_type);

  return &type;
}

Type *TypeAnalysis::visitDefineStructTypeInst(DefineStructTypeInst &I) {
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

Type *TypeAnalysis::visitStructTypeInst(StructTypeInst &I) {
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

Type *TypeAnalysis::visitStaticTensorTypeInst(StaticTensorTypeInst &I) {
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

Type *TypeAnalysis::visitTensorTypeInst(TensorTypeInst &I) {
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not determine element of TensorType");

  // Build the TensorType.
  auto &type = Type::get_tensor_type(elem_type, I.getNumberOfDimensions());

  return &type;
}

Type *TypeAnalysis::visitAssocArrayTypeInst(AssocArrayTypeInst &I) {
  auto &key_type = MEMOIR_SANITIZE(this->analyze(I.getKeyOperand()),
                                   "Could not determine key of AssocType");
  auto &value_type = MEMOIR_SANITIZE(this->analyze(I.getValueOperand()),
                                     "Could not determine value of AssocType");

  // Build the AssocArrayType.
  auto &type = AssocArrayType::get(key_type, value_type);

  return &type;
}

Type *TypeAnalysis::visitSequenceTypeInst(SequenceTypeInst &I) {
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not determine element of SequenceType");

  auto &type = SequenceType::get(elem_type);

  return &type;
}

// Allocation instructions.
Type *TypeAnalysis::visitStructAllocInst(StructAllocInst &I) {
  // Get the struct type.
  return this->analyze(I.getTypeOperand());
}

Type *TypeAnalysis::visitTensorAllocInst(TensorAllocInst &I) {

  // Determine the element type.
  auto &element_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not find element type of TensorAlloc!");

  // Build the TensorType.
  auto &type = TensorType::get(element_type, I.getNumberOfDimensions());

  return &type;
}

Type *TypeAnalysis::visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
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

Type *TypeAnalysis::visitSequenceAllocInst(SequenceAllocInst &I) {
  // Get the element type.
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not determine SequenceAlloc element type!");

  // Create the sequence type.
  auto &seq_type = SequenceType::get(elem_type);

  return &seq_type;
}

// Reference Read Instructions.
Type *TypeAnalysis::visitReadInst(ReadInst &I) {
  // The result has no memoir type, it's an LLVM value.
  return nullptr;
}

// Nested Access Instructions.
Type *TypeAnalysis::visitGetInst(GetInst &I) {
  // Get the type of collection being accessed.
  auto *object_type = this->analyze(I.getObjectOperand());
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(object_type),
                      "GetInst accessing non-collection type!");

  // Return the element type.
  auto &element_type = collection_type.getElementType();
  return &element_type;
}

// Write access instructions.
Type *TypeAnalysis::visitWriteInst(WriteInst &I) {
  return this->analyze(I.getObjectOperand());
}

// SSA Instructions
Type *TypeAnalysis::visitUsePHIInst(UsePHIInst &I) {
  return this->analyze(I.getUsedCollection());
}

Type *TypeAnalysis::visitDefPHIInst(DefPHIInst &I) {
  return this->analyze(I.getDefinedCollection());
}

Type *TypeAnalysis::visitArgPHIInst(ArgPHIInst &I) {
  return this->analyze(I.getInputCollection());
}

Type *TypeAnalysis::visitRetPHIInst(RetPHIInst &I) {
  return this->analyze(I.getInputCollection());
}

// SSA collection operations.
Type *TypeAnalysis::visitCopyInst(CopyInst &I) {
  return this->analyze(I.getCopiedCollection());
}

Type *TypeAnalysis::visitInsertInst(InsertInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeAnalysis::visitRemoveInst(RemoveInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeAnalysis::visitSwapInst(SwapInst &I) {
  return this->analyze(I.getFromCollection());
}

// SSA assoc operations.
Type *TypeAnalysis::visitAssocInsertInst(AssocInsertInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeAnalysis::visitAssocRemoveInst(AssocRemoveInst &I) {
  return this->analyze(I.getBaseCollection());
}

Type *TypeAnalysis::visitAssocHasInst(AssocHasInst &I) {
  // We _could_ use this opportunity to unify with an abstract assoc, but we
  // won't unless deemed necessary.
  return nullptr;
}

Type *TypeAnalysis::visitAssocKeysInst(AssocKeysInst &I) {

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
Type *TypeAnalysis::visitExtractValueInst(llvm::ExtractValueInst &I) {
  // Get the aggregate type.
  auto &aggregate =
      MEMOIR_SANITIZE(I.getAggregateOperand(),
                      "Aggregate operand of ExtractValueInst is NULL!");

  // We will simply say that aggregate types _always_ hold the same type as its
  // operands. This is a fine assumption becuase, so far, only seq swap produces
  // an aggregate.
  return this->analyze(aggregate);
}

Type *TypeAnalysis::visitLoadInst(llvm::LoadInst &I) {
  // If we have load instruction, trace back to its global variable and find the
  // original store to it.
  auto *load_ptr = I.getPointerOperand();
  auto *global = dyn_cast<GlobalVariable>(load_ptr);
  if (!global) {
    if (auto *load_gep = dyn_cast<GetElementPtrInst>(load_ptr)) {
      auto *gep_ptr = load_gep->getPointerOperand();
      global = dyn_cast<GlobalVariable>(gep_ptr);
    }
  }

  // If we still cannot find the GlobalVariable, return NULL.
  if (!global) {
    return nullptr;
  }

  // Find the original store for this global variable.
  for (auto *user : global->users()) {
    if (auto *store_inst = dyn_cast<StoreInst>(user)) {
      auto *store_value = store_inst->getValueOperand();

      auto *stored_type = this->analyze(*store_value);
      MEMOIR_NULL_CHECK(stored_type,
                        "Could not determine the type being loaded");

      return stored_type;
    }
  }

  // Otherwise, return NULL.
  return nullptr;
}

Type *TypeAnalysis::visitPHINode(llvm::PHINode &I) {

  // Check that we have not already visited this PHI.
  auto found = value_bindings.find(&I);
  if (found != value_bindings.end()) {
    // If we have, return its temporary binding.
    return found->second;
  }

  // Create a type variable for the PHI node.
  auto &phi_type = this->new_type_variable();

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

// Union-find
TypeVariable &TypeAnalysis::new_type_variable() {
  auto *typevar = new TypeVariable(this->current_id++);

  this->type_bindings[typevar] = typevar;

  return *typevar;
}

Type *TypeAnalysis::find(Type *t) {
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

Type *TypeAnalysis::unify(Type *t, Type *u) {
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
TypeAnalysis::TypeAnalysis() : current_id(0) {}

// Destructor
TypeAnalysis::~TypeAnalysis() {
  for (const auto &[typevar, type] : type_bindings) {
    delete typevar;
  }
}

} // namespace llvm::memoir
