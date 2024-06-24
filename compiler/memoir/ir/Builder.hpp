#ifndef MEMOIR_BUILDER_H
#define MEMOIR_BUILDER_H

#include <initializer_list>
#include <type_traits>

#include "llvm/IR/IRBuilder.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/MutOperations.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

class MemOIRBuilder : public IRBuilder<> {
public:
  /*
   * MemOIRBuilder Constructors
   */
  MemOIRBuilder(llvm::BasicBlock *BB) : IRBuilder<>(BB), M(BB->getModule()) {
    MEMOIR_NULL_CHECK(this->M,
                      "Could not determine LLVM Module of basic block.");
  }

  MemOIRBuilder(llvm::Instruction *IP, bool InsertAfter = false)
    : IRBuilder<>((InsertAfter) ? IP->getNextNode() : IP),
      M(IP->getModule()) {
    MEMOIR_NULL_CHECK(this->M,
                      "Could not determine LLVM Module of insertion point.");
  }

  MemOIRBuilder(MemOIRInst &IP, bool InsertAfter = false)
    : MemOIRBuilder(&IP.getCallInst(), InsertAfter) {}

  llvm::Module &getModule() {
    return *(this->M);
  }

  /*
   * Type Instructions
   */
  TypeInst *CreateTypeInst(Type &type, const Twine &name = "") {
    if (isa<FloatType>(&type)) {
      return this->CreateFloatTypeInst(name);
    } else if (isa<DoubleType>(&type)) {
      return this->CreateDoubleTypeInst(name);
    } else if (isa<PointerType>(&type)) {
      return this->CreatePointerTypeInst(name);
    } else if (auto *integer_type = dyn_cast<IntegerType>(&type)) {
      if (!integer_type->isSigned()) {
        switch (integer_type->getBitWidth()) {
          case 64:
            return this->CreateUInt64TypeInst(name);
          case 32:
            return this->CreateUInt32TypeInst(name);
          case 16:
            return this->CreateUInt16TypeInst(name);
          case 8:
            return this->CreateUInt8TypeInst(name);
          default:
            MEMOIR_UNREACHABLE(
                "Attempt to create unknown unsigned integer type!");
        }
      } else {
        switch (integer_type->getBitWidth()) {
          case 64:
            return this->CreateInt64TypeInst(name);
          case 32:
            return this->CreateInt32TypeInst(name);
          case 16:
            return this->CreateInt16TypeInst(name);
          case 8:
            return this->CreateInt8TypeInst(name);
          case 2:
            return this->CreateInt2TypeInst(name);
          case 1:
            return this->CreateBoolTypeInst(name);
          default:
            MEMOIR_UNREACHABLE(
                "Attempt to create unknown signed integer type!");
        }
      }
    } else if (auto *ref_type = dyn_cast<ReferenceType>(&type)) {
      return this->CreateReferenceTypeInst(
          &this->CreateTypeInst(ref_type->getReferencedType(), name)
               ->getCallInst(),
          name);
    } else if (auto *struct_type = dyn_cast<StructType>(&type)) {
      auto *name_global = &struct_type->getDefinition().getNameOperand();
      if (auto *name_as_inst = dyn_cast<llvm::Instruction>(name_global)) {
        name_global = name_as_inst->clone();
        this->Insert(cast<llvm::Instruction>(name_global));
      }
      return this->CreateStructTypeInst(name_global, name);
    } else if (auto *field_array_type = dyn_cast<FieldArrayType>(&type)) {
      return this->CreateTypeInst(field_array_type->getStructType());
    } else if (auto *static_tensor_type = dyn_cast<StaticTensorType>(&type)) {
      MEMOIR_UNREACHABLE("CreateStaticTensorType is unimplemented!");
    } else if (auto *tensor_type = dyn_cast<TensorType>(&type)) {
      MEMOIR_UNREACHABLE("CreateTensorType is unimplemented!");
    } else if (auto *assoc_type = dyn_cast<AssocArrayType>(&type)) {
      return this->CreateAssocArrayTypeInst(
          &this->CreateTypeInst(assoc_type->getKeyType())->getCallInst(),
          &this->CreateTypeInst(assoc_type->getValueType())->getCallInst(),
          name);
    } else if (auto *seq_type = dyn_cast<SequenceType>(&type)) {
      return this->CreateSequenceTypeInst(
          &this->CreateTypeInst(seq_type->getElementType())->getCallInst());
    }
    MEMOIR_UNREACHABLE("Attempt to create instruction for unknown type");
  }

  /*
   * Primitive Type Instructions
   */
#define HANDLE_PRIMITIVE_TYPE_INST(ENUM, FUNC, CLASS)                          \
  CLASS *Create##CLASS(const Twine &name = "") {                               \
    return this->create<CLASS>(MemOIR_Func::ENUM, {}, name);                   \
  }
#include "memoir/ir/Instructions.def"

  // Derived Type Instructions
  DefineStructTypeInst *CreateDefineStructTypeInst(
      const char *type_name,
      int num_fields,
      vector<llvm::Value *> field_types,
      const Twine &name = "") {
    // Create the LLVM type name and number of fields constant.
    auto llvm_type_name = this->CreateGlobalString(type_name, "type.struct.");
    auto llvm_num_fields = this->getInt64(num_fields);

    // Build the list of arguments.
    auto llvm_args = vector<llvm::Value *>({ llvm_type_name, llvm_num_fields });
    for (auto field_type : field_types) {
      llvm_args.push_back(field_type);
    }

    // Create the call.
    return this->create<DefineStructTypeInst>(MemOIR_Func::DEFINE_STRUCT_TYPE,
                                              llvm::ArrayRef(llvm_args),
                                              name);
  }

  StructTypeInst *CreateStructTypeInst(llvm::Value *llvm_type_name,
                                       const Twine &name = "") {

    return this->create<StructTypeInst>(MemOIR_Func::STRUCT_TYPE,
                                        { llvm_type_name },
                                        name);
  }

  ReferenceTypeInst *CreateReferenceTypeInst(llvm::Value *referenced_type,
                                             const Twine &name = "") {
    return this->create<ReferenceTypeInst>(MemOIR_Func::REFERENCE_TYPE,
                                           { referenced_type },
                                           name);
  }

  SequenceTypeInst *CreateSequenceTypeInst(llvm::Value *element_type,
                                           const Twine &name = "") {
    return this->create<SequenceTypeInst>(MemOIR_Func::SEQUENCE_TYPE,
                                          { element_type },
                                          name);
  }

  AssocArrayTypeInst *CreateAssocArrayTypeInst(llvm::Value *key_type,
                                               llvm::Value *value_type,
                                               const Twine &name = "") {
    return this->create<AssocArrayTypeInst>(MemOIR_Func::ASSOC_ARRAY_TYPE,
                                            { key_type, value_type },
                                            name);
  }

  // TODO: Add the other derived type instructions.

  // Allocation Instructions
  SequenceAllocInst *CreateSequenceAllocInst(Type &type,
                                             uint64_t size,
                                             const Twine &name = "") {
    return this->CreateSequenceAllocInst(
        &this->CreateTypeInst(type)->getCallInst(),
        size,
        name);
  }

  SequenceAllocInst *CreateSequenceAllocInst(Type &type,
                                             llvm::Value *size,
                                             const Twine &name = "") {
    return this->CreateSequenceAllocInst(
        &this->CreateTypeInst(type)->getCallInst(),
        size,
        name);
  }

  SequenceAllocInst *CreateSequenceAllocInst(llvm::Value *type,
                                             uint64_t size,
                                             const Twine &name = "") {
    return this->CreateSequenceAllocInst(type, this->getInt64(size), name);
  }

  SequenceAllocInst *CreateSequenceAllocInst(llvm::Value *type,
                                             llvm::Value *size,
                                             const Twine &name = "") {
    return this->create<SequenceAllocInst>(MemOIR_Func::ALLOCATE_SEQUENCE,
                                           { type, size },
                                           name);
  }

  // Assoc allocation
  AssocArrayAllocInst *CreateAssocArrayAllocInst(Type &key_type,
                                                 Type &value_type,
                                                 const Twine &name = "") {
    return this->CreateAssocArrayAllocInst(
        &this->CreateTypeInst(key_type)->getCallInst(),
        &this->CreateTypeInst(value_type)->getCallInst(),
        name);
  }

  AssocArrayAllocInst *CreateAssocArrayAllocInst(Type &key_type,
                                                 llvm::Value *value_type,
                                                 const Twine &name = "") {
    return this->CreateAssocArrayAllocInst(
        &this->CreateTypeInst(key_type)->getCallInst(),
        value_type,
        name);
  }

  AssocArrayAllocInst *CreateAssocArrayAllocInst(llvm::Value *key_type,
                                                 llvm::Value *value_type,
                                                 const Twine &name = "") {
    return this->create<AssocArrayAllocInst>(MemOIR_Func::ALLOCATE_ASSOC_ARRAY,
                                             { key_type, value_type },
                                             name);
  }

  // Access Instructions
  StructWriteInst *CreateStructWriteInst(Type &element_type,
                                         llvm::Value *llvm_value_to_write,
                                         llvm::Value *llvm_collection,
                                         llvm::Value *llvm_field_index,
                                         const Twine &name = "") {
    return this->create<StructWriteInst>(
        getStructWriteEnumForType(element_type),
        { llvm_value_to_write, llvm_collection, llvm_field_index },
        name);
  }

  IndexWriteInst *CreateIndexWriteInst(Type &element_type,
                                       llvm::Value *llvm_value_to_write,
                                       llvm::Value *llvm_collection,
                                       llvm::Value *llvm_index,
                                       const Twine &name = "") {
    return this->create<IndexWriteInst>(
        getIndexWriteEnumForType(element_type),
        { llvm_value_to_write, llvm_collection, llvm_index },
        name);
  }

  MutIndexWriteInst *CreateMutIndexWriteInst(Type &element_type,
                                             llvm::Value *llvm_value_to_write,
                                             llvm::Value *llvm_collection,
                                             llvm::Value *llvm_index,
                                             const Twine &name = "") {
    return this->create<MutIndexWriteInst>(
        getMutIndexWriteEnumForType(element_type),
        { llvm_value_to_write, llvm_collection, llvm_index },
        name);
  }

  AssocWriteInst *CreateAssocWriteInst(Type &element_type,
                                       llvm::Value *llvm_value_to_write,
                                       llvm::Value *llvm_collection,
                                       llvm::Value *llvm_assoc,
                                       const Twine &name = "") {
    return this->create<AssocWriteInst>(
        getAssocWriteEnumForType(element_type),
        { llvm_value_to_write, llvm_collection, llvm_assoc },
        name);
  }

  MutAssocWriteInst *CreateMutAssocWriteInst(Type &element_type,
                                             llvm::Value *llvm_value_to_write,
                                             llvm::Value *llvm_collection,
                                             llvm::Value *llvm_assoc,
                                             const Twine &name = "") {
    return this->create<MutAssocWriteInst>(
        getMutAssocWriteEnumForType(element_type),
        { llvm_value_to_write, llvm_collection, llvm_assoc },
        name);
  }

  // Deletion Instructions
  DeleteStructInst *CreateDeleteStructInst(llvm::Value *struct_to_delete) {
    return this->create<DeleteStructInst>(MemOIR_Func::DELETE_STRUCT,
                                          { struct_to_delete });
  }

  DeleteCollectionInst *CreateDeleteCollectionInst(
      llvm::Value *collection_to_delete) {
    return this->create<DeleteCollectionInst>(MemOIR_Func::DELETE_COLLECTION,
                                              { collection_to_delete });
  }

  // Assoc operations.
  //// SSA assoc operations.
  AssocInsertInst *CreateAssocInsertInst(llvm::Value *collection,
                                         llvm::Value *key_value,
                                         const Twine &name = "") {
    return this->create<AssocInsertInst>(MemOIR_Func::ASSOC_INSERT,
                                         { collection, key_value },
                                         name);
  }

  AssocRemoveInst *CreateAssocRemoveInst(llvm::Value *collection,
                                         llvm::Value *key_value,
                                         const Twine &name = "") {
    return this->create<AssocRemoveInst>(
        MemOIR_Func::ASSOC_REMOVE,
        llvm::ArrayRef({ collection, key_value }),
        name);
  }

  //// Mutable assoc operations.
  MutAssocInsertInst *CreateMutAssocInsertInst(llvm::Value *collection,
                                               llvm::Value *key_value,
                                               const Twine &name = "") {
    return this->create<MutAssocInsertInst>(MemOIR_Func::MUT_ASSOC_INSERT,
                                            { collection, key_value },
                                            name);
  }

  MutAssocRemoveInst *CreateMutAssocRemoveInst(llvm::Value *collection,
                                               llvm::Value *key_value,
                                               const Twine &name = "") {
    return this->create<MutAssocRemoveInst>(MemOIR_Func::MUT_ASSOC_REMOVE,
                                            { collection, key_value },
                                            name);
  }

  // Sequence operations.
  //// SSA sequence operations.
  SeqSwapInst *CreateSeqSwapInst(llvm::Value *from_collection,
                                 llvm::Value *from_begin,
                                 llvm::Value *from_end,
                                 llvm::Value *to_collection,
                                 llvm::Value *to_begin,
                                 const Twine &name = "") {
    return this->create<SeqSwapInst>(
        MemOIR_Func::SEQ_SWAP,
        { from_collection, from_begin, from_end, to_collection, to_begin },
        name);
  }

  SeqSwapWithinInst *CreateSeqSwapWithinInst(llvm::Value *collection,
                                             llvm::Value *from_begin,
                                             llvm::Value *from_end,
                                             llvm::Value *to_begin,
                                             const Twine &name = "") {
    return this->create<SeqSwapWithinInst>(
        MemOIR_Func::SEQ_SWAP_WITHIN,
        { collection, from_begin, from_end, to_begin },
        name);
  }
  SeqRemoveInst *CreateSeqRemoveInst(llvm::Value *collection,
                                     llvm::Value *begin,
                                     llvm::Value *end,
                                     const Twine &name = "") {
    return this->create<SeqRemoveInst>(MemOIR_Func::SEQ_REMOVE,
                                       { collection, begin, end },
                                       name);
  }

  SeqInsertInst *CreateSeqInsertInst(Type &element_type,
                                     llvm::Value *llvm_value_to_write,
                                     llvm::Value *llvm_collection,
                                     llvm::Value *llvm_index,
                                     const Twine &name = "") {
    return this->create<SeqInsertInst>(
        getSeqInsertEnumForType(element_type),
        { llvm_value_to_write, llvm_collection, llvm_index },
        name);
  }

  SeqInsertSeqInst *CreateSeqInsertSeqInst(llvm::Value *collection_to_insert,
                                           llvm::Value *collection,
                                           llvm::Value *insertion_point,
                                           const Twine &name = "") {
    return this->create<SeqInsertSeqInst>(
        MemOIR_Func::SEQ_INSERT,
        { collection_to_insert, collection, insertion_point },
        name);
  }

  SeqCopyInst *CreateSeqCopyInst(llvm::Value *collection,
                                 llvm::Value *left,
                                 llvm::Value *right,
                                 const Twine &name = "") {
    return this->create<SeqCopyInst>(MemOIR_Func::SEQ_COPY,
                                     { collection, left, right },
                                     name);
  }

  SeqCopyInst *CreateSeqCopyInst(llvm::Value *collection_to_slice,
                                 llvm::Value *left,
                                 int64_t right,
                                 const Twine &name = "") {
    // Create the right constant.
    auto right_constant = this->getInt64(right);

    // Call the base builder method.
    return this->CreateSeqCopyInst(collection_to_slice,
                                   left,
                                   right_constant,
                                   name);
  }

  SeqCopyInst *CreateSeqCopyInst(llvm::Value *collection_to_slice,
                                 int64_t left,
                                 llvm::Value *right,
                                 const Twine &name = "") {
    // Create the left constant.
    auto left_constant = this->getInt64(left);

    // Call the base builder method.
    return this->CreateSeqCopyInst(collection_to_slice,
                                   left_constant,
                                   right,
                                   name);
  }

  SeqCopyInst *CreateSeqCopyInst(llvm::Value *collection_to_slice,
                                 int64_t left,
                                 int64_t right,
                                 const Twine &name = "") {
    // Create the left constant.
    auto left_constant = this->getInt64(left);

    // Create the right constant.
    auto right_constant = this->getInt64(right);

    // Call the base builder method.
    return this->CreateSeqCopyInst(collection_to_slice,
                                   left_constant,
                                   right_constant,
                                   name);
  }

  //// Mutable sequence operations.
  MutSeqAppendInst *CreateMutSeqAppendInst(llvm::Value *collection,
                                           llvm::Value *collection_to_append,
                                           const Twine &name = "") {
    return this->create<MutSeqAppendInst>(MemOIR_Func::MUT_SEQ_APPEND,
                                          { collection, collection_to_append },
                                          name);
  }

  MutSeqSwapInst *CreateMutSeqSwapInst(llvm::Value *from_collection,
                                       llvm::Value *from_begin,
                                       llvm::Value *from_end,
                                       llvm::Value *to_collection,
                                       llvm::Value *to_begin,
                                       const Twine &name = "") {
    return this->create<MutSeqSwapInst>(
        MemOIR_Func::MUT_SEQ_SWAP,
        { from_collection, from_begin, from_end, to_collection, to_begin },
        name);
  }

  MutSeqSwapWithinInst *CreateMutSeqSwapWithinInst(llvm::Value *collection,
                                                   llvm::Value *from_begin,
                                                   llvm::Value *from_end,
                                                   llvm::Value *to_begin,
                                                   const Twine &name = "") {
    return this->create<MutSeqSwapWithinInst>(
        MemOIR_Func::MUT_SEQ_SWAP_WITHIN,
        { collection, from_begin, from_end, collection, to_begin },
        name);
  }

  MutSeqRemoveInst *CreateMutSeqRemoveInst(llvm::Value *collection,
                                           llvm::Value *begin,
                                           llvm::Value *end,
                                           const Twine &name = "") {
    return this->create<MutSeqRemoveInst>(MemOIR_Func::MUT_SEQ_REMOVE,
                                          { collection, begin, end },
                                          name);
  }

  MutSeqInsertInst *CreateMutSeqInsertInst(Type &element_type,
                                           llvm::Value *llvm_value_to_write,
                                           llvm::Value *llvm_collection,
                                           llvm::Value *llvm_index,
                                           const Twine &name = "") {
    return this->create<MutSeqInsertInst>(
        getMutSeqInsertEnumForType(element_type),
        { llvm_value_to_write, llvm_collection, llvm_index },
        name);
  }

  MutSeqInsertSeqInst *CreateMutSeqInsertSeqInst(
      llvm::Value *collection_to_insert,
      llvm::Value *collection,
      llvm::Value *insertion_point,
      const Twine &name = "") {
    return this->create<MutSeqInsertSeqInst>(
        MemOIR_Func::MUT_SEQ_INSERT,
        { collection_to_insert, collection, insertion_point },
        name);
  }

  // General-purpose collection operations.
  //// Size operations.
  SizeInst *CreateSizeInst(llvm::Value *collection_to_slice,
                           const Twine &name = "") {
    return this->create<SizeInst>(MemOIR_Func::SIZE,
                                  { collection_to_slice },
                                  name);
  }

  EndInst *CreateEndInst(const Twine &name = "") {
    return this->create<EndInst>(MemOIR_Func::END, {}, name);
  }

  //// SSA renaming operations.
  UsePHIInst *CreateUsePHI(llvm::Value *collection, const Twine &name = "") {
    return this->create<UsePHIInst>(MemOIR_Func::USE_PHI, { collection }, name);
  }

  DefPHIInst *CreateDefPHI(llvm::Value *collection, const Twine &name = "") {
    return this->create<DefPHIInst>(MemOIR_Func::DEF_PHI, { collection }, name);
  }

  // Type annotations.
  MemOIRInst *CreateAssertTypeInst(llvm::Value *object,
                                   Type &type,
                                   const Twine &name = "") {
    // If the type is a CollectionType, create an AssertCollectionTypeInst.
    if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
      return this->CreateAssertCollectionTypeInst(object,
                                                  *collection_type,
                                                  name);
    }

    // If the type is a StructType, create an AssertStructTypeInst.
    if (auto *struct_type = dyn_cast<StructType>(&type)) {
      return this->CreateAssertStructTypeInst(object, *struct_type, name);
    }

    MEMOIR_UNREACHABLE("Unknown type to assert!");
  }

  AssertStructTypeInst *CreateAssertStructTypeInst(llvm::Value *strct,
                                                   Type &type,
                                                   const Twine &name = "") {
    // Create the type instruction.
    auto *type_inst = CreateTypeInst(type, name);

    // Create the type annotation.
    return this->create<AssertStructTypeInst>(
        MemOIR_Func::ASSERT_STRUCT_TYPE,
        { &type_inst->getCallInst(), strct });
  }

  AssertCollectionTypeInst *CreateAssertCollectionTypeInst(
      llvm::Value *collection,
      Type &type,
      const Twine &name = "") {
    // Create the type instruction.
    auto &type_inst = MEMOIR_SANITIZE(CreateTypeInst(type, name),
                                      "Could not construct type instruction!");

    // Create the type annotation.
    return this->create<AssertCollectionTypeInst>(
        MemOIR_Func::ASSERT_COLLECTION_TYPE,
        { &type_inst.getCallInst(), collection });
  }

  ReturnTypeInst *CreateReturnTypeInst(llvm::Value *type_as_value,
                                       const Twine &name = "") {
    // Create the type annotation.
    return this->create<ReturnTypeInst>(MemOIR_Func::SET_RETURN_TYPE,
                                        { type_as_value },
                                        name);
  }

  ReturnTypeInst *CreateReturnTypeInst(Type &type, const Twine &name = "") {
    // Create the type instruction.
    auto *type_inst = CreateTypeInst(type, name);

    // Create the type annotation.
    return this->CreateReturnTypeInst(&type_inst->getCallInst(), name);
  }

protected:
  // Borrowed state
  llvm::Module *M;

  // Helper Functions
  template <typename T,
            std::enable_if_t<std::is_base_of_v<MemOIRInst, T>, bool> = true>
  T *create(MemOIR_Func memoir_enum,
            std::initializer_list<llvm::Value *> arguments = {},
            const Twine &name = "") {
    return this->create<T>(memoir_enum,
                           llvm::ArrayRef<llvm::Value *>(arguments),
                           name);
  }

  template <typename T,
            std::enable_if_t<std::is_base_of_v<MemOIRInst, T>, bool> = true>
  T *create(MemOIR_Func memoir_enum,
            llvm::ArrayRef<llvm::Value *> arguments = {},
            const Twine &name = "") {
    // Use the general-purpose helper to construct the call.
    auto *llvm_call = this->createMemOIRCall(memoir_enum, arguments, name);

    // Cast to MemOIRInst and return.
    auto inst = into<T>(llvm_call);
    MEMOIR_NULL_CHECK(inst, "Type mismatch for result of llvm call!");

    // Return.
    return inst;
  }

  llvm::CallInst *createMemOIRCall(MemOIR_Func memoir_enum,
                                   llvm::ArrayRef<llvm::Value *> arguments = {},
                                   const Twine &name = "") {
    // Fetch the LLVM Function.
    auto *llvm_func =
        FunctionNames::get_memoir_function(*(this->M), memoir_enum);

    // Create the function callee.
    auto callee = FunctionCallee(llvm_func);

    // Get the function type so that we know if it's okay to name the resultant.
    auto *llvm_func_type = callee.getFunctionType();
    auto *return_type =
        llvm_func_type == nullptr ? nullptr : llvm_func_type->getReturnType();

    // Create the call.
    auto *llvm_call = (return_type == nullptr || return_type->isVoidTy())
                          ? this->CreateCall(callee, arguments)
                          : this->CreateCall(callee, arguments, name);

    MEMOIR_NULL_CHECK(llvm_call,
                      "Error creating LLVM call to memoir function!");

    // Return.
    return llvm_call;
  }

#define ENUM_FOR_PRIMITIVE_TYPE(ENUM_PREFIX, NAME)                             \
  MemOIR_Func get##NAME##EnumForType(Type &type) {                             \
    if (isa<FloatType>(&type)) {                                               \
      return MemOIR_Func::ENUM_PREFIX##_FLOAT;                                 \
    } else if (isa<DoubleType>(&type)) {                                       \
      return MemOIR_Func::ENUM_PREFIX##_DOUBLE;                                \
    } else if (isa<PointerType>(&type)) {                                      \
      return MemOIR_Func::ENUM_PREFIX##_PTR;                                   \
    } else if (isa<ReferenceType>(&type)) {                                    \
      return MemOIR_Func::ENUM_PREFIX##_STRUCT_REF;                            \
    } else if (auto *integer_type = dyn_cast<IntegerType>(&type)) {            \
      if (!integer_type->isSigned()) {                                         \
        switch (integer_type->getBitWidth()) {                                 \
          case 64:                                                             \
            return MemOIR_Func::ENUM_PREFIX##_UINT64;                          \
          case 32:                                                             \
            return MemOIR_Func::ENUM_PREFIX##_UINT32;                          \
          case 16:                                                             \
            return MemOIR_Func::ENUM_PREFIX##_UINT16;                          \
          case 8:                                                              \
            return MemOIR_Func::ENUM_PREFIX##_UINT8;                           \
          default:                                                             \
            MEMOIR_UNREACHABLE(                                                \
                "Attempt to create unknown unsigned integer type!");           \
        }                                                                      \
      } else {                                                                 \
        switch (integer_type->getBitWidth()) {                                 \
          case 64:                                                             \
            return MemOIR_Func::ENUM_PREFIX##_INT64;                           \
          case 32:                                                             \
            return MemOIR_Func::ENUM_PREFIX##_INT32;                           \
          case 16:                                                             \
            return MemOIR_Func::ENUM_PREFIX##_INT16;                           \
          case 8:                                                              \
            return MemOIR_Func::ENUM_PREFIX##_INT8;                            \
          case 2:                                                              \
            return MemOIR_Func::ENUM_PREFIX##_INT2;                            \
          case 1:                                                              \
            return MemOIR_Func::ENUM_PREFIX##_BOOL;                            \
          default:                                                             \
            MEMOIR_UNREACHABLE(                                                \
                "Attempt to create unknown signed integer type!");             \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    MEMOIR_UNREACHABLE("Attempt to create instruction for unknown type");      \
  };

  ENUM_FOR_PRIMITIVE_TYPE(INDEX_READ, IndexRead)
  ENUM_FOR_PRIMITIVE_TYPE(ASSOC_READ, AssocRead)
  ENUM_FOR_PRIMITIVE_TYPE(STRUCT_READ, StructRead)
  ENUM_FOR_PRIMITIVE_TYPE(INDEX_WRITE, IndexWrite)
  ENUM_FOR_PRIMITIVE_TYPE(ASSOC_WRITE, AssocWrite)
  ENUM_FOR_PRIMITIVE_TYPE(STRUCT_WRITE, StructWrite)
  ENUM_FOR_PRIMITIVE_TYPE(SEQ_INSERT, SeqInsert)
  ENUM_FOR_PRIMITIVE_TYPE(MUT_INDEX_WRITE, MutIndexWrite)
  ENUM_FOR_PRIMITIVE_TYPE(MUT_ASSOC_WRITE, MutAssocWrite)
  ENUM_FOR_PRIMITIVE_TYPE(MUT_STRUCT_WRITE, MutStructWrite)
  ENUM_FOR_PRIMITIVE_TYPE(MUT_SEQ_INSERT, MutSeqInsert)

#define ENUM_FOR_NESTED_TYPE(ENUM_PREFIX, NAME)                                \
  MemOIR_Func get##NAME##EnumForType(Type &type) {                             \
    if (isa<StructType>(&type)) {                                              \
      return MemOIR_Func::ENUM_PREFIX##_STRUCT;                                \
    } else if (isa<CollectionType>(&type)) {                                   \
      return MemOIR_Func::ENUM_PREFIX##_COLLECTION;                            \
    }                                                                          \
    MEMOIR_UNREACHABLE("Attempt to create instruction for unknown type");      \
  };

  ENUM_FOR_NESTED_TYPE(INDEX_GET, IndexGet)
  ENUM_FOR_NESTED_TYPE(ASSOC_GET, AssocGet)
  ENUM_FOR_NESTED_TYPE(STRUCT_GET, StructGet)

}; // namespace llvm::memoir

} // namespace llvm::memoir

#endif
