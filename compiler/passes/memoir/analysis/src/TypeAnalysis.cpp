#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// Initialization.
TypeAnalysis::TypeAnalysis() {
  // Do nothing.
}

// Helper macros.
#define CHECK_MEMOIZED(V)                                                      \
  /* See if an existing type exists, if it does, early return. */              \
  if (auto found = this->findExisting(V)) {                                    \
    return found;                                                              \
  }                                                                            \
  if (this->beenVisited(V)) {                                                  \
    return nullptr;                                                            \
  }                                                                            \
  this->markVisited(V);                                                        \
  debugln("TypeAnalysis: visiting ", V);

#define MEMOIZE(V, T)                                                          \
  /* Memoize the type */                                                       \
  this->memoize(V, T);

#define MEMOIZE_AND_RETURN(V, T)                                               \
  debugln("TypeAnalysis: done ", V);                                           \
  /* Memoize */                                                                \
  MEMOIZE(V, T)                                                                \
  /* Return */                                                                 \
  return T

// Queries.
Type *TypeAnalysis::analyze(llvm::Value &V) {
  return TypeAnalysis::get().getType(V);
}

// Analysis.
Type *TypeAnalysis::getType(llvm::Value &V) {
  this->visited.clear();
  this->visited_edges.clear();
  // this->edge_types.clear();

  return getType_helper(V);
}

Type *TypeAnalysis::getType_helper(llvm::Value &V) {
  // If we have an instruction, visit it.
  // If the instruction has a type, return it.
  if (auto inst = dyn_cast<llvm::Instruction>(&V)) {
    if (auto *type = this->visit(*inst)) {
      return type;
    }
  }

  // If this is a constant NULL, return null.
  if (isa<llvm::ConstantPointerNull>(&V)) {
    return nullptr;
  }

  // Otherwise, check if this value is used by an assert type.
  for (auto *user : V.users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      continue;
    }

    if (auto *memoir_inst = MemOIRInst::get(*user_as_inst)) {
      if (auto *assert_struct_type_inst =
              dyn_cast<AssertStructTypeInst>(memoir_inst)) {
        if (auto *type =
                this->visitAssertStructTypeInst(*assert_struct_type_inst)) {
          MEMOIZE_AND_RETURN(V, type);
        }
      } else if (auto *assert_collection_type_inst =
                     dyn_cast<AssertCollectionTypeInst>(memoir_inst)) {
        if (auto *type = this->visitAssertCollectionTypeInst(
                *assert_collection_type_inst)) {
          MEMOIZE_AND_RETURN(V, type);
        }
      }
    }
  }

  return nullptr;
}

// Base case.
Type *TypeAnalysis::visitInstruction(llvm::Instruction &I) {
  return nullptr;
}

// TypeInsts.
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

Type *TypeAnalysis::visitUInt2TypeInst(UInt2TypeInst &I) {
  return &(Type::get_u2_type());
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

Type *TypeAnalysis::visitInt2TypeInst(Int2TypeInst &I) {
  return &(Type::get_i2_type());
}

Type *TypeAnalysis::visitBoolTypeInst(BoolTypeInst &I) {
  return &(Type::get_bool_type());
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

  // Get the types of each field.
  vector<Type *> field_types;
  for (auto field_idx = 0; field_idx < I.getNumberOfFields(); field_idx++) {
    auto &field_type_value = I.getFieldTypeOperand(field_idx);
    auto *field_type = this->getType_helper(field_type_value);
    field_types.push_back(field_type);
  }

  // Build the StructType
  auto &type = StructType::define(I, I.getName(), field_types);

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitStructTypeInst(StructTypeInst &I) {
  CHECK_MEMOIZED(I);

  // Get all users of the given name.
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

  // For each user, find the call to define the struct type.
  for (auto *call : call_inst_users) {
    if (FunctionNames::is_memoir_call(*call)) {
      auto func_enum = FunctionNames::get_memoir_enum(*call);

      if (func_enum == MemOIR_Func::DEFINE_STRUCT_TYPE) {
        auto defined_type = this->getType_helper(*call);
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

  // Get the length of dimensions.
  auto num_dimensions = I.getNumberOfDimensions();

  vector<size_t> length_of_dimensions;
  length_of_dimensions.resize(num_dimensions);

  for (auto dim_idx = 0; dim_idx < num_dimensions; dim_idx++) {
    length_of_dimensions[dim_idx] = I.getLengthOfDimension(dim_idx);
  }

  // Build the new type.
  auto &type =
      Type::get_static_tensor_type(I.getElementType(), length_of_dimensions);

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitTensorTypeInst(TensorTypeInst &I) {
  CHECK_MEMOIZED(I);

  // Build the TensorType.
  auto &type =
      Type::get_tensor_type(I.getElementType(), I.getNumberOfDimensions());

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitAssocArrayTypeInst(AssocArrayTypeInst &I) {
  CHECK_MEMOIZED(I);

  // Build the AssocArrayType.
  auto &type = AssocArrayType::get(I.getKeyType(), I.getValueType());

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitSequenceTypeInst(SequenceTypeInst &I) {
  CHECK_MEMOIZED(I);

  // Build the SequenceType.
  auto &type = SequenceType::get(I.getElementType());

  MEMOIZE_AND_RETURN(I, &type);
}

// Allocation instructions.
Type *TypeAnalysis::visitStructAllocInst(StructAllocInst &I) {
  CHECK_MEMOIZED(I);

  // Recurse on the type operand.
  auto type = this->getType_helper(I.getTypeOperand());

  MEMOIR_NULL_CHECK(type,
                    "Could not determine the struct type being allocated");

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitTensorAllocInst(TensorAllocInst &I) {
  CHECK_MEMOIZED(I);

  // Determine the element type.
  auto element_type = this->getType_helper(I.getElementOperand());
  MEMOIR_NULL_CHECK(element_type, "Element type of tensor allocation is NULL");

  // Build the TensorType.
  auto &type = Type::get_tensor_type(*element_type, I.getNumberOfDimensions());

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
  CHECK_MEMOIZED(I);

  // Determine the Key and Value types.
  auto key_type = this->getType_helper(I.getKeyOperand());
  MEMOIR_NULL_CHECK(key_type, "Key type of assoc array allocation is NULL");
  auto value_type = this->getType_helper(I.getValueOperand());
  MEMOIR_NULL_CHECK(value_type, "Value type of assoc array allocation is NULL");

  // Build the AssocArrayType.
  auto &type = Type::get_assoc_array_type(*key_type, *value_type);

  MEMOIZE_AND_RETURN(I, &type);
}

Type *TypeAnalysis::visitSequenceAllocInst(SequenceAllocInst &I) {
  CHECK_MEMOIZED(I);

  // Determine the element type.
  auto element_type = this->getType_helper(I.getElementOperand());
  MEMOIR_NULL_CHECK(element_type, "Element type of sequence is NULL");

  // Build the SequenceType.
  auto &type = Type::get_sequence_type(*element_type);

  MEMOIZE_AND_RETURN(I, &type);
}

// Reference Read Instructions.
Type *TypeAnalysis::visitReadInst(ReadInst &I) {
  CHECK_MEMOIZED(I);

  // Determine the type of the read collection's elements.
  auto &collection_type = I.getCollectionType();
  auto &element_type = collection_type.getElementType();

  // If the element type is a reference type, unwrap it.
  if (auto reference_type = dyn_cast<ReferenceType>(&element_type)) {
    auto &referenced_type = reference_type->getReferencedType();
    MEMOIZE_AND_RETURN(I, &referenced_type);
  }

  // Otherwise, return the bare element type.
  MEMOIZE_AND_RETURN(I, &element_type);
}

// Nested Access Instructions.
Type *TypeAnalysis::visitGetInst(GetInst &I) {
  CHECK_MEMOIZED(I);

  // Determine the type of the nested collection.
  auto &collection_type = I.getCollectionType();
  auto &nested_type = collection_type.getElementType();

  MEMOIZE_AND_RETURN(I, &nested_type);
}

// SSA Instructions
Type *TypeAnalysis::visitUsePHIInst(UsePHIInst &I) {
  CHECK_MEMOIZED(I);

  // Visit the used collection to determine its type.
  auto *type = this->getType_helper(I.getUsedCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitDefPHIInst(DefPHIInst &I) {
  CHECK_MEMOIZED(I);

  // Visit the defined collection to determine its type.
  auto *type = this->getType_helper(I.getDefinedCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitArgPHIInst(ArgPHIInst &I) {
  CHECK_MEMOIZED(I);

  // Visit the incoming collection to determine its type.
  auto *type = this->getType_helper(I.getInputCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitRetPHIInst(RetPHIInst &I) {
  CHECK_MEMOIZED(I);

  // Visit the incoming collection to determine its type.
  auto *type = this->getType_helper(I.getInputCollection());

  MEMOIZE_AND_RETURN(I, type);
}

// SSA collection operations.
Type *TypeAnalysis::visitCopyInst(CopyInst &I) {
  CHECK_MEMOIZED(I);

  // Determine the type of the copied collection.
  auto type = this->getType_helper(I.getCopiedCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitInsertInst(InsertInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getBaseCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitRemoveInst(RemoveInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getBaseCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitSwapInst(SwapInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getFromCollection());

  if (type == nullptr) {
    type = this->getType_helper(I.getToCollection());

    // Forward the type information along.
    MEMOIZE(I.getFromCollection(), type);
  } else {
    // Forward the type information along.
    MEMOIZE(I.getToCollection(), type);
  }

  MEMOIZE_AND_RETURN(I, type);
}

// Mutable sequence operations.
Type *TypeAnalysis::visitMutSeqInsertInst(MutSeqInsertInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitMutSeqRemoveInst(MutSeqRemoveInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitMutSeqSwapInst(MutSeqSwapInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getFromCollection());

  if (type == nullptr) {
    type = this->getType_helper(I.getToCollection());

    // Forward the type information along.
    MEMOIZE(I.getFromCollection(), type);
  } else {
    // Forward the type information along.
    MEMOIZE(I.getToCollection(), type);
  }

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitMutSeqSwapWithinInst(MutSeqSwapWithinInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitMutSeqAppendInst(MutSeqAppendInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitMutSeqSplitInst(MutSeqSplitInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

// Lowering sequence operations.
Type *TypeAnalysis::visitViewInst(ViewInst &I) {
  CHECK_MEMOIZED(I);

  // Determine the type of the viewed collection.
  auto type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

// SSA assoc operations.
Type *TypeAnalysis::visitAssocHasInst(AssocHasInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getObjectOperand());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitAssocKeysInst(AssocKeysInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());
  auto *assoc_type = dyn_cast_or_null<AssocArrayType>(type);
  MEMOIR_NULL_CHECK(assoc_type,
                    "Collection passed to assoc_keys is not an assoc");
  auto &key_type = assoc_type->getKeyType();

  auto &key_seq_type = Type::get_sequence_type(key_type);

  MEMOIZE_AND_RETURN(I, &key_seq_type);
}

// Mut assoc operations.
Type *TypeAnalysis::visitMutAssocInsertInst(MutAssocInsertInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitMutAssocRemoveInst(MutAssocRemoveInst &I) {
  CHECK_MEMOIZED(I);

  auto *type = this->getType_helper(I.getCollection());

  MEMOIZE_AND_RETURN(I, type);
}

// Type Checking instruction.
Type *TypeAnalysis::visitAssertStructTypeInst(AssertStructTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType_helper(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitAssertCollectionTypeInst(AssertCollectionTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType_helper(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::visitReturnTypeInst(ReturnTypeInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Determine the type being checked.
   */
  auto type = this->getType_helper(I.getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Type being asserted is NULL");

  MEMOIZE_AND_RETURN(I, type);
}

// LLVM Instructions
Type *TypeAnalysis::visitLLVMCallInst(llvm::CallInst &I) {
  CHECK_MEMOIZED(I);

  // See if this CallInst returns a pointer type
  auto function_type = I.getFunctionType();
  MEMOIR_NULL_CHECK(function_type, "Found a call with NULL function type");

  auto llvm_return_type = function_type->getReturnType();
  MEMOIR_NULL_CHECK(llvm_return_type, "Found a call with NULL return type");

  if (!isa<llvm::PointerType>(llvm_return_type)) {
    return nullptr;
  }

  // Analyze the call.
  Type *return_type = nullptr;
  auto callee = I.getCalledFunction();
  if (callee) {
    // Handle direct call.
    return_type = this->getReturnType(*callee);
  } else {
    // Handle indirect call.
    auto parent_bb = I.getParent();
    MEMOIR_NULL_CHECK(parent_bb,
                      "Could not determine the parent basic block of the call");
    auto parent_function = parent_bb->getParent();
    MEMOIR_NULL_CHECK(parent_function,
                      "Could not determine the parent function of the call");
    auto parent_module = parent_function->getParent();
    MEMOIR_NULL_CHECK(parent_module,
                      "Could not determine the parent module of the call");

    set<Type *> returned_types; // should have a single item
    for (auto &F : *parent_module) {
      if (F.getFunctionType() != function_type) {
        continue;
      }

      returned_types.insert(this->getReturnType(F));
    }

    MEMOIR_ASSERT((returned_types.size() == 1),
                  "Could not determine the return type for indirect call!");
  }

  MEMOIZE_AND_RETURN(I, return_type);
}

Type *TypeAnalysis::visitExtractValueInst(llvm::ExtractValueInst &I) {
  CHECK_MEMOIZED(I);

  // Get the aggregate operand.
  auto &aggregate =
      MEMOIR_SANITIZE(I.getAggregateOperand(),
                      "Aggregate operand of ExtractValueInst is NULL!");

  // Recurse on the aggregate.
  auto *type = this->getType_helper(aggregate);

  MEMOIZE_AND_RETURN(I, type);
}

Type *TypeAnalysis::getReturnType(llvm::Function &F) {
  if (F.empty()) {
    return nullptr;
  }

  // See if we have a to ReturnTypeInst we can use.
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *memoir_inst = MemOIRInst::get(I)) {
        if (auto *return_type_inst = dyn_cast<ReturnTypeInst>(memoir_inst)) {
          return this->getType_helper(return_type_inst->getTypeOperand());
        }
      }
    }
  }

  // Otherwise, inspect all of the return statements to get the type.
  set<Type *> returned_types; // should have a single item.
  for (auto &BB : F) {
    auto *terminator = BB.getTerminator();
    if (auto *return_inst = dyn_cast<llvm::ReturnInst>(terminator)) {
      auto *return_value = return_inst->getReturnValue();
      if (!return_value) {
        returned_types.insert(nullptr);
        continue;
      }
      returned_types.insert(this->getType_helper(*return_value));
    }
  }

  MEMOIR_ASSERT(returned_types.size() == 1,
                "Could not determine a static type for the function!");

  return *(returned_types.begin());
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
      auto store_value = store_inst->getValueOperand();

      auto stored_type = this->getType_helper(*store_value);
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

  // See if we already have an edge set for this PHI node
  auto &incoming_types = this->edge_types[&I];

  // Get all incoming types.
  for (auto &incoming : I.incoming_values()) {
    // Get the incoming value.
    auto &incoming_value = *(incoming.get());

    // Check if the incoming value has already been visited
    if (this->visited_edges[&I].count(&incoming_value) > 0) {
      continue;
    }

    // Add the incoming value to the list of visited edges.
    this->visited_edges[&I].insert(&incoming_value);

    // Get the Type of the incoming value.
    auto *incoming_type = this->getType_helper(incoming_value);

    // Insert the incoming Type into the set of incoming types.
    // NOTE: there should only be ONE type in the set after this loop.
    if (incoming_type != nullptr) {
      incoming_types.insert(incoming_type);
    }
  }

  // Check that all incoming types are the same.
  if (incoming_types.size() != 1) {
    warnln("Could not statically determine the type of PHI node.");
    return nullptr;
  }

  // Get the single Type.
  auto *type = *(incoming_types.begin());

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

bool TypeAnalysis::beenVisited(llvm::Value &V) {
  return (this->visited.count(&V) > 0);
}

bool TypeAnalysis::beenVisited(MemOIRInst &I) {
  return this->beenVisited(I.getCallInst());
}

void TypeAnalysis::markVisited(llvm::Value &V) {
  this->visited.insert(&V);
  return;
}

void TypeAnalysis::markVisited(MemOIRInst &I) {
  this->markVisited(I.getCallInst());
  return;
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
