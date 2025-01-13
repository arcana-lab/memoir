#ifndef MEMOIR_BUILDER_H
#define MEMOIR_BUILDER_H

#include <initializer_list>
#include <type_traits>

#include "llvm/IR/IRBuilder.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Keywords.hpp"
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

  llvm::LLVMContext &getContext() {
    return this->getModule().getContext();
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
    } else if (isa<VoidType>(&type)) {
      return this->CreateVoidTypeInst(name);
    } else if (auto *integer_type = dyn_cast<IntegerType>(&type)) {
      if (not integer_type->isSigned()) {
        switch (integer_type->getBitWidth()) {
          case 64:
            return this->CreateUInt64TypeInst(name);
          case 32:
            return this->CreateUInt32TypeInst(name);
          case 16:
            return this->CreateUInt16TypeInst(name);
          case 8:
            return this->CreateUInt8TypeInst(name);
          case 1:
            return this->CreateBoolTypeInst(name);
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
    } else if (auto *array_type = dyn_cast<ArrayType>(&type)) {
      MEMOIR_UNREACHABLE("CreateArrayType is unimplemented!");
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

  TypeInst *CreateSizeTypeInst(const Twine &name = "") {
    auto &data_layout = this->M->getDataLayout();
    auto &size_type = Type::get_size_type(data_layout);
    return this->CreateTypeInst(size_type, name);
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
  AllocInst *CreateAllocInst(llvm::Value *type,
                             const vector<llvm::Value *> &extra_args = {},
                             const Twine &name = "") {
    vector<llvm::Value *> args = { type };
    args.insert(args.end(), extra_args.begin(), extra_args.end());

    return this->create<AllocInst>(MemOIR_Func::ALLOCATE, args, name);
  }

  AllocInst *CreateAllocInst(Type &type,
                             const vector<llvm::Value *> &extra_args = {},
                             const Twine &name = "") {
    auto &type_as_value = this->CreateTypeInst(type)->getCallInst();
    return this->CreateAllocInst(&type_as_value, extra_args, name);
  }

  // Sequence allocation
  AllocInst *CreateSequenceAllocInst(Type &type,
                                     uint64_t size,
                                     const Twine &name = "") {
    return this->CreateSequenceAllocInst(type, this->getInt64(size), name);
  }

  AllocInst *CreateSequenceAllocInst(Type &type,
                                     llvm::Value *size,
                                     const Twine &name = "") {
    return this->CreateSequenceAllocInst(
        &this->CreateTypeInst(type)->getCallInst(),
        size,
        name);
  }

  AllocInst *CreateSequenceAllocInst(llvm::Value *type,
                                     uint64_t size,
                                     const Twine &name = "") {
    return this->CreateSequenceAllocInst(type, this->getInt64(size), name);
  }

  AllocInst *CreateSequenceAllocInst(llvm::Value *type,
                                     llvm::Value *size,
                                     const Twine &name = "") {
    return this->CreateAllocInst(type, { size }, name);
  }

  // Assoc allocation
  AllocInst *CreateAssocArrayAllocInst(Type &key_type,
                                       Type &value_type,
                                       const Twine &name = "") {
    return this->CreateAssocArrayAllocInst(
        &this->CreateTypeInst(key_type)->getCallInst(),
        &this->CreateTypeInst(value_type)->getCallInst(),
        name);
  }

  AllocInst *CreateAssocArrayAllocInst(Type &key_type,
                                       llvm::Value *value_type,
                                       const Twine &name = "") {
    return this->CreateAssocArrayAllocInst(
        &this->CreateTypeInst(key_type)->getCallInst(),
        value_type,
        name);
  }

  AllocInst *CreateAssocArrayAllocInst(llvm::Value *key_type,
                                       llvm::Value *value_type,
                                       const Twine &name = "") {
    auto &type_as_value =
        this->CreateAssocArrayTypeInst(key_type, value_type)->getCallInst();
    return this->CreateAllocInst(&type_as_value, {}, name);
  }

  // Access Instructions

  //// Read Instructions
  ReadInst *CreateReadInst(Type &element_type,
                           llvm::Value *llvm_object,
                           const vector<llvm::Value *> &indices = {},
                           const Twine &name = "") {
    vector<llvm::Value *> args = { llvm_object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<ReadInst>(getReadEnumForType(element_type), args, name);
  }

  //// Get Instructions
  GetInst *CreateGetInst(llvm::Value *llvm_object,
                         const vector<llvm::Value *> &indices = {},
                         const Twine &name = "") {
    vector<llvm::Value *> args = { llvm_object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<GetInst>(MemOIR_Func::GET, args, name);
  }

  //// Write Instructions.
  WriteInst *CreateWriteInst(Type &element_type,
                             llvm::Value *llvm_value_to_write,
                             llvm::Value *llvm_object,
                             const vector<llvm::Value *> &indices = {},
                             const Twine &name = "") {
    vector<llvm::Value *> args = { llvm_value_to_write, llvm_object };
    args.insert(args.end(), indices.cbegin(), indices.cend());

    return this->create<WriteInst>(getWriteEnumForType(element_type),
                                   args,
                                   name);
  }

  DeleteInst *CreateDeleteInst(llvm::Value *to_delete) {
    return this->create<DeleteInst>(MemOIR_Func::DELETE, { to_delete });
  }

  InsertInst *CreateInsertInst(llvm::Value *object,
                               const vector<llvm::Value *> &indices,
                               const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<InsertInst>(MemOIR_Func::INSERT, args, name);
  }

  RemoveInst *CreateRemoveInst(llvm::Value *object,
                               const vector<llvm::Value *> &indices,
                               const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<RemoveInst>(MemOIR_Func::REMOVE, args, name);
  }

  KeysInst *CreateKeysInst(llvm::Value *object,
                           const vector<llvm::Value *> &indices = {},
                           const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<KeysInst>(MemOIR_Func::KEYS, args, name);
  }

  HasInst *CreateHasInst(llvm::Value *object,
                         const vector<llvm::Value *> &indices,
                         const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<HasInst>(MemOIR_Func::HAS, args, name);
  }

  //// Mutable operations.
  MutWriteInst *CreateMutWriteInst(Type &element_type,
                                   llvm::Value *llvm_value_to_write,
                                   llvm::Value *llvm_object,
                                   const vector<llvm::Value *> &indices = {},
                                   const Twine &name = "") {
    vector<llvm::Value *> args = { llvm_value_to_write, llvm_object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<MutWriteInst>(getMutWriteEnumForType(element_type),
                                      args,
                                      name);
  }

  MutInsertInst *CreateMutInsertInst(llvm::Value *object,
                                     const vector<llvm::Value *> &indices,
                                     const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<MutInsertInst>(MemOIR_Func::MUT_INSERT, args, name);
  }

  MutRemoveInst *CreateMutRemoveInst(llvm::Value *object,
                                     const vector<llvm::Value *> &indices,
                                     const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), indices.begin(), indices.end());

    return this->create<MutRemoveInst>(MemOIR_Func::MUT_REMOVE, args, name);
  }

  // Sequence operations.
  CopyInst *CreateCopyInst(llvm::Value *object,
                           const vector<llvm::Value *> &extra_args = {},
                           const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), extra_args.begin(), extra_args.end());

    return this->create<CopyInst>(MemOIR_Func::COPY, args, name);
  }

  CopyInst *CreateSeqCopyInst(llvm::Value *object,
                              llvm::Value *left,
                              llvm::Value *right,
                              const Twine &name = "") {
    vector<llvm::Value *> args = { &Keyword::get_llvm<RangeKeyword>(
                                       this->getContext()),
                                   left,
                                   right };

    // Call the base builder method.
    return this->CreateCopyInst(object, args, name);
  }

  CopyInst *CreateSeqCopyInst(llvm::Value *object,
                              llvm::Value *left,
                              int64_t right,
                              const Twine &name = "") {
    // Create the right constant.
    auto right_constant = this->getInt64(right);

    // Call the base builder method.
    return this->CreateSeqCopyInst(object, left, right_constant, name);
  }

  CopyInst *CreateSeqCopyInst(llvm::Value *object,
                              int64_t left,
                              llvm::Value *right,
                              const Twine &name = "") {
    // Create the left constant.
    auto left_constant = this->getInt64(left);

    // Call the base builder method.
    return this->CreateSeqCopyInst(object, left_constant, right, name);
  }

  CopyInst *CreateSeqCopyInst(llvm::Value *object,
                              int64_t left,
                              int64_t right,
                              const Twine &name = "") {
    // Create the left constant.
    auto left_constant = this->getInt64(left);

    // Create the right constant.
    auto right_constant = this->getInt64(right);

    // Call the base builder method.
    return this->CreateSeqCopyInst(object, left_constant, right_constant, name);
  }

  // General-purpose collection operations.
  //// Size operations.
  SizeInst *CreateSizeInst(llvm::Value *object,
                           const vector<llvm::Value *> &extra_args = {},
                           const Twine &name = "") {
    vector<llvm::Value *> args = { object };
    args.insert(args.end(), extra_args.begin(), extra_args.end());

    return this->create<SizeInst>(MemOIR_Func::SIZE, args, name);
  }

  EndInst *CreateEndInst(const Twine &name = "") {
    return this->create<EndInst>(MemOIR_Func::END, {}, name);
  }

  //// Fold operation
  FoldInst *CreateFoldInst(Type &type,
                           llvm::Value *initial,
                           llvm::Value *collection,
                           llvm::Function *body,
                           llvm::ArrayRef<llvm::Value *> closed = {},
                           const Twine &name = "") {
    // Fetch the function type.
    auto *func_type = body->getFunctionType();

    // Construct the call.
    return this->CreateFoldInst(getFoldEnumForType(type),
                                initial,
                                collection,
                                body,
                                closed,
                                name);
  }

  FoldInst *CreateFoldInst(MemOIR_Func fold_enum,
                           llvm::Value *initial,
                           llvm::Value *collection,
                           llvm::Function *body,
                           llvm::ArrayRef<llvm::Value *> closed = {},
                           const Twine &name = "") {
    // Fetch the function type.
    auto *func_type = body->getFunctionType();

    // Create the argument list.
    vector<llvm::Value *> args = { initial, collection, body };
    if (not closed.empty()) {
      args.push_back(&Keyword::get_llvm<ClosedKeyword>(this->getContext()));
      args.insert(args.end(), closed.begin(), closed.end());
    }

    // Construct the call.
    return this->create<FoldInst>(fold_enum,
                                  llvm::ArrayRef<llvm::Value *>(args),
                                  name);
  }

  //// SSA renaming operations.
  UsePHIInst *CreateUsePHI(llvm::Value *collection, const Twine &name = "") {
    return this->create<UsePHIInst>(MemOIR_Func::USE_PHI, { collection }, name);
  }

  RetPHIInst *CreateRetPHI(llvm::Value *collection,
                           llvm::Value *callee,
                           const Twine &name = "") {
    return this->create<RetPHIInst>(MemOIR_Func::RET_PHI,
                                    { collection, callee },
                                    name);
  }

  ClearInst *CreateClearInst(llvm::Value *collection,
                             const vector<llvm::Value *> &extra_args = {},
                             const Twine &name = "") {
    vector<llvm::Value *> arguments = { collection };
    arguments.insert(arguments.end(), extra_args.begin(), extra_args.end());
    return this->create<ClearInst>(MemOIR_Func::CLEAR, { collection }, name);
  }

  // Type annotations.
  AssertTypeInst *CreateAssertTypeInst(llvm::Value *object,
                                       Type &type,
                                       const Twine &name = "") {
    // Create the type instruction.
    auto &type_inst = MEMOIR_SANITIZE(CreateTypeInst(type, name),
                                      "Could not construct type instruction!");

    // Create the type annotation.
    return this->create<AssertTypeInst>(MemOIR_Func::ASSERT_TYPE,
                                        { &type_inst.getCallInst(), object });
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
      return MemOIR_Func::ENUM_PREFIX##_REF;                                   \
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
          case 1:                                                              \
            return MemOIR_Func::ENUM_PREFIX##_BOOL;                            \
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

  ENUM_FOR_PRIMITIVE_TYPE(READ, Read)
  ENUM_FOR_PRIMITIVE_TYPE(WRITE, Write)
  ENUM_FOR_PRIMITIVE_TYPE(MUT_WRITE, MutWrite)
#undef ENUM_FOR_PRIMITIVE_TYPE

#define ENUM_FOR_TYPE(ENUM_PREFIX, NAME)                                       \
  MemOIR_Func get##NAME##EnumForType(Type &type) {                             \
    if (isa<FloatType>(&type)) {                                               \
      return MemOIR_Func::ENUM_PREFIX##_FLOAT;                                 \
    } else if (isa<DoubleType>(&type)) {                                       \
      return MemOIR_Func::ENUM_PREFIX##_DOUBLE;                                \
    } else if (isa<PointerType>(&type)) {                                      \
      return MemOIR_Func::ENUM_PREFIX##_PTR;                                   \
    } else if (isa<CollectionType>(&type)) {                                   \
      return MemOIR_Func::ENUM_PREFIX##_REF;                                   \
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
          case 1:                                                              \
            return MemOIR_Func::ENUM_PREFIX##_BOOL;                            \
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
  }

  ENUM_FOR_TYPE(FOLD, Fold)
  ENUM_FOR_TYPE(RFOLD, ReverseFold)
#undef ENUM_FOR_TYPE

}; // namespace llvm::memoir

} // namespace llvm::memoir

#endif
