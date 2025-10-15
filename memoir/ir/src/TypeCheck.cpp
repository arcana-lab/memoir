#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Print.hpp"

namespace memoir {

// Top-level query.
Type *TypeChecker::type_of(MemOIRInst &I) {
  return TypeChecker::type_of(I.getCallInst());
}

Type *TypeChecker::type_of(llvm::Value &V) {
  TypeChecker checker;

  // Only type check LLVM pointers.
  if (not isa<llvm::PointerType>(V.getType())) {
    return nullptr;
  }

  // Get the type of this value.
  auto *type = checker.analyze(V);

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

  // Otherwise, we don't know the type of this argument.
  // Create a type variable for it in case other local information can give us
  // the information we need.
  auto *type_var = &this->new_type_variable();
  this->value_bindings[&A] = type_var;

  return type_var;
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

Type *TypeChecker::visitVoidTypeInst(VoidTypeInst &I) {
  return &VoidType::get();
}

Type *TypeChecker::visitReferenceTypeInst(ReferenceTypeInst &I) {
  auto &referenced_type =
      MEMOIR_SANITIZE(this->analyze(I.getReferencedTypeOperand()),
                      "Could not determine referenced type!");

  auto &type = Type::get_ref_type(referenced_type);

  return &type;
}

Type *TypeChecker::visitTupleTypeInst(TupleTypeInst &I) {
  // Get the types of each field.
  Vector<Type *> field_types;
  for (unsigned field_idx = 0; field_idx < I.getNumberOfFields(); ++field_idx) {
    auto &field_type_value = I.getFieldTypeOperand(field_idx);
    auto *field_type = this->analyze(field_type_value);
    field_types.push_back(field_type);
  }

  // Build the TupleType
  auto &type = TupleType::get(field_types);

  return &type;
}

// Type *TypeChecker::visitDefineTypeInst(DefineTypeInst &I) {
//   // Get the type being defined.
//   auto &type = this->analyze(this->getTypeOperand());

//   // Define the TupleType
//   TupleType::define(I, I.getName(), type);

//   return &type;
// }

// Type *TypeChecker::visitTupleTypeInst(TupleTypeInst &I) {
//   // Get all users of the given name.
//   auto &name_value = MEMOIR_SANITIZE(
//       I.getNameOperand().stripPointerCasts(),
//       "Could not get the name operand stripped of pointer casts");

//   Set<DefineTupleTypeInst *> call_inst_users = {};
//   for (auto *user : name_value.users()) {

//     if (auto *type_def = into<DefineTupleTypeInst>(user)) {
//       call_inst_users.insert(type_def);
//     } else if (auto *user_as_gep = dyn_cast<llvm::GetElementPtrInst>(user)) {
//       for (auto *gep_user : user_as_gep->users()) {
//         if (auto *type_def = into<DefineTupleTypeInst>(gep_user)) {
//           call_inst_users.insert(type_def);
//         }
//       }
//     } else if (auto *user_as_gep = dyn_cast<llvm::ConstantExpr>(user)) {
//       for (auto *gep_user : user_as_gep->users()) {
//         if (auto *type_def = into<DefineTupleTypeInst>(gep_user)) {
//           call_inst_users.insert(type_def);
//         }
//       }
//     }
//   }

//   // For each user, find the call to define the struct type.
//   for (auto *call : call_inst_users) {
//     auto *defined_type = this->visitDefineTupleTypeInst(*call);
//     MEMOIR_NULL_CHECK(defined_type,
//                       "Could not determine the defined struct type");
//     return defined_type;
//   }

//   MEMOIR_UNREACHABLE(
//       "Could not find a definition for the given struct type name");
// }

Type *TypeChecker::visitArrayTypeInst(ArrayTypeInst &I) {
  // Get the element type.
  auto &elem_type = MEMOIR_SANITIZE(this->analyze(I.getElementTypeOperand()),
                                    "Could not determine element of ArrayType");

  auto length = I.getLength();

  // Build the new type.
  auto &type = Type::get_array_type(elem_type, length);

  return &type;
}

Type *TypeChecker::visitAssocArrayTypeInst(AssocArrayTypeInst &I) {
  auto &key_type = MEMOIR_SANITIZE(this->analyze(I.getKeyOperand()),
                                   "Could not determine key of AssocType");
  auto &value_type = MEMOIR_SANITIZE(this->analyze(I.getValueOperand()),
                                     "Could not determine value of AssocType");

  if (auto selection_kw = I.get_keyword<SelectionKeyword>()) {
    return &AssocArrayType::get(key_type,
                                value_type,
                                selection_kw->getSelection());
  }

  return &AssocArrayType::get(key_type, value_type);
}

Type *TypeChecker::visitSequenceTypeInst(SequenceTypeInst &I) {
  auto &elem_type =
      MEMOIR_SANITIZE(this->analyze(I.getElementOperand()),
                      "Could not determine element of SequenceType ",
                      I.getElementOperand());

  if (auto selection_kw = I.get_keyword<SelectionKeyword>()) {
    return &SequenceType::get(elem_type, selection_kw->getSelection());
  }

  return &SequenceType::get(elem_type);
}

// Allocation instructions.
Type *TypeChecker::visitAllocInst(AllocInst &I) {
  return this->analyze(I.getTypeOperand());
}

// Access instructions.
Type *TypeChecker::visitAccessInst(AccessInst &I) {
  return &I.getElementType();
}

Type *TypeChecker::visitReadInst(ReadInst &I) {
  auto &element_type = I.getElementType();

  // If the element type is a ReferenceType, unpack and return it.
  if (auto *ref_type = dyn_cast<ReferenceType>(&element_type)) {
    return &ref_type->getReferencedType();
  }

  // Otherwise, return NULL, as its an LLVM type.
  return nullptr;
}

Type *TypeChecker::visitHasInst(HasInst &I) {
  // We _could_ use this opportunity to unify with an abstract assoc, but we
  // won't unless deemed necessary.
  return nullptr;
}

Type *TypeChecker::nested_type(AccessInst &access) {
  // Get the base type of the accessed object.
  auto *type = this->analyze(access.getObject());

  // Find the nested type based on the access indices.
  for (auto *index : access.indices()) {
    if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      // Get the nested element type.
      type = &collection_type->getElementType();

    } else if (auto *tuple_type = dyn_cast<TupleType>(type)) {
      // Get the index constant.
      auto *index_constant = dyn_cast<llvm::ConstantInt>(index);
      MEMOIR_ASSERT(index_constant, "Tuple access with non-constant index.");
      auto field = index_constant->getZExtValue();

      // Get the nested field type.
      auto &field_type = tuple_type->getFieldType(field);
      type = &field_type;

    } else {
      MEMOIR_UNREACHABLE("Indexing into non-object type: ", *type);
    }
  }

  return type;
}

Type *TypeChecker::visitCopyInst(CopyInst &I) {
  return this->nested_type(I);
}

// Update instructions.
Type *TypeChecker::visitUpdateInst(UpdateInst &I) {
  return this->analyze(I.getObject());
}

// SSA collection operations.
Type *TypeChecker::visitFoldInst(FoldInst &I) {
  return this->analyze(I.getInitial());
}

Type *TypeChecker::visitKeysInst(KeysInst &I) {
  // Get the type of the inner referenced collection.
  auto &inner_type = I.getElementType();

  // Cast it to an assoc type, if it is not one, error!
  auto &assoc_type = MEMOIR_SANITIZE(dyn_cast<AssocArrayType>(&inner_type),
                                     "Keys used on non-assoc operand.");

  // Fetch the key type.
  auto &key_type = assoc_type.getKeyType();

  // Construct a sequence type for it.
  auto &seq_type = SequenceType::get(key_type);

  return &seq_type;
}

// SSA Instructions
Type *TypeChecker::visitUsePHIInst(UsePHIInst &I) {
  return this->analyze(I.getUsed());
}

Type *TypeChecker::visitRetPHIInst(RetPHIInst &I) {
  return this->analyze(I.getInput());
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
  auto *ptr = I.getPointerOperand();
  // See if the loaded value is used in any type assertions.
  for (auto &use : I.uses()) {
    if (auto *user_as_inst = dyn_cast<llvm::Instruction>(use.getUser())) {
      if (auto *assert_type = into<AssertTypeInst>(user_as_inst)) {
        if (&I == &assert_type->getObject()) {
          return &assert_type->getType();
        }
      }
    }
  }

  // Find any stores for this variable and unify based on them.
  for (auto *user : ptr->users()) {
    if (auto *store_inst = dyn_cast<llvm::StoreInst>(user)) {
      auto *store_value = store_inst->getValueOperand();

      auto *stored_type = this->analyze(*store_value);

      // TODO: we should unify here instead of taking the first result, but this
      // will do fine for now.
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
  auto *type_var = &this->new_type_variable();
  this->value_bindings[&I] = type_var;

  // For each incoming value, visit it, and unify it with the PHI type.
  for (auto &incoming : I.incoming_values()) {
    auto &incoming_value =
        MEMOIR_SANITIZE(incoming.get(), "Found a NULL value when typing!");
    auto *incoming_type = this->analyze(incoming_value);

    // Unify the PHI with the incoming type.
    auto unified_type = this->unify(type_var, incoming_type);
    MEMOIR_ASSERT(unified_type, "Failed to unify type of ", I);
  }

  // Find the resulting type of the PHI and return it.
  return this->find(type_var);
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
  auto &typevar = TypeVariable::get();

  this->type_bindings[&typevar] = &typevar;

  return typevar;
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

Option<Type *> TypeChecker::unify(Type *t, Type *u) {
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

  if (t != nullptr) {
    if (auto *tvar = dyn_cast<TypeVariable>(t)) {
      this->type_bindings[tvar] = u;
      return u;
    }
  } else {
    return u;
  }

  if (u != nullptr) {
    if (auto *uvar = dyn_cast<TypeVariable>(u)) {
      this->type_bindings[uvar] = t;
      return t;
    }
  } else {
    return t;
  }

  if (t) {
    println("t = ", *t);
  } else {
    println("t = NULL");
  }
  if (u) {
    println("u = ", *u);
  } else {
    println("u = NULL");
  }

  // If neither t nor u is a type variable, and they are not equal, we
  // have a type error!
  return {};
}

// Constructor
TypeChecker::TypeChecker() : current_id(0) {}

// Destructor
TypeChecker::~TypeChecker() {
  for (const auto &[typevar, type] : type_bindings) {
    delete typevar;
  }
}

} // namespace memoir
