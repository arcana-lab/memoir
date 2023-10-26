#include "memoir/utility/FunctionNames.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/lowering/TypeLayout.hpp"

#include "SSADestruction.hpp"

#define COLLECTION_SELECTION 1
#if !defined(COLLECTION_SELECTION)
#  define COLLECTION_SELECTION 0
#endif

// #define ASSOC_IMPL "hashtable"
// #define ASSOC_IMPL "stl_unordered_map"
// #define ASSOC_IMPL "stl_map"
// #define ASSOC_IMPL "deepsjeng_ttable"
// #define ASSOC_IMPL "llvm_densemap"
#define ASSOC_IMPL "llvm_smallptrset"

// #define SEQ_IMPL "vector"
// #define SEQ_IMPL "stl_vector"
#define SEQ_IMPL "llvm_smallvector"

namespace llvm::memoir {

SSADestructionVisitor::SSADestructionVisitor(llvm::Module &M,
                                             SSADestructionStats *stats)
  : M(M),
    TC(M.getContext()),
    stats(stats) {
  // Do nothing.
}

void SSADestructionVisitor::setAnalyses(llvm::noelle::DomTreeSummary &DT,
                                        LivenessAnalysis &LA,
                                        ValueNumbering &VN) {
  this->DT = &DT;
  this->LA = &LA;
  this->VN = &VN;

  return;
}

void SSADestructionVisitor::visitInstruction(llvm::Instruction &I) {
  return;
}

void SSADestructionVisitor::visitSequenceAllocInst(SequenceAllocInst &I) {
#if COLLECTION_SELECTION
  // TODO: run escape analysis to determine if we can do a stack allocation.
  // auto escaped = this->EA.escapes(I);
  bool escaped = true;

  auto &element_type = I.getElementType();

  auto element_code = element_type.get_code();
  auto operation = escaped ? "allocate" : "initialize";
  auto impl_prefix = *element_code + "_" SEQ_IMPL;
  auto name = impl_prefix + "__" + operation;

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    println("Couldn't find vector alloc for ", name);
    MEMOIR_UNREACHABLE("see above");
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *vector_size = &I.getSizeOperand();

  llvm::CallInst *llvm_call;
  if (escaped) {
    llvm_call =
        builder.CreateCall(function_callee, llvm::ArrayRef({ vector_size }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector alloc");
  } else {
    // Create/fetch the struct type.
    llvm::StructType *struct_type = nullptr;
    if (function != nullptr) {
      auto *param_type = function->getFunctionType()->getParamType(0);
      auto *ptr_type = dyn_cast<llvm::PointerType>(param_type);
      auto *elem_type = ptr_type->getElementType();
      struct_type = dyn_cast<llvm::StructType>(elem_type);
    } else if (struct_type == nullptr) {
      auto struct_name = "struct." + impl_prefix + "_t";
      struct_type = llvm::StructType::create(M.getContext(), struct_name);
    }
    MEMOIR_NULL_CHECK(
        struct_type,
        "Could not find or create the LLVM StructType for " ASSOC_IMPL "!");

    // Create a stack location.
    auto *llvm_alloca = builder.CreateAlloca(struct_type);

    // Initialize the stack location.
    llvm_call = builder.CreateCall(
        function_callee,
        llvm::ArrayRef<llvm::Value *>({ llvm_alloca, vector_size }));
  }

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

  this->coalesce(I, *collection);
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
#if COLLECTION_SELECTION
  // TODO: run escape analysis to determine if we can do a stack allocation.
  // auto escaped = this->EA.escapes(I);
  bool escaped = true;

  auto &key_type = I.getKeyType();
  auto &value_type = I.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto impl_prefix = *key_code + "_" + *value_code + "_" ASSOC_IMPL;
  auto operation = escaped ? "allocate" : "initialize";
  auto name = impl_prefix + "__" + operation;

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (isa<CollectionType>(&value_type)) {
    // TODO: emit the needed implementation to the linker.
    auto &value_type_layout = TC.convert(value_type);
    auto &llvm_value_type = value_type_layout.get_llvm_type();

    auto &data_layout = M.getDataLayout();
    auto llvm_value_size = data_layout.getTypeAllocSize(&llvm_value_type);
    println("value type size = ", llvm_value_size);
    if (auto *arr_type = dyn_cast<llvm::ArrayType>(&llvm_value_type)) {
      println("  element size = ",
              data_layout.getTypeAllocSize(arr_type->getElementType()));
    }
  }

  if (function == nullptr) {
    println("Couldn't find assoc alloc for ", name);
    MEMOIR_UNREACHABLE("see above");
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();

  llvm::CallInst *llvm_call;

  if (escaped) {
    llvm_call = builder.CreateCall(function_callee);
  } else {
    // Create/fetch the struct type.
    llvm::StructType *struct_type = nullptr;
    if (function != nullptr) {
      auto *param_type = function->getFunctionType()->getParamType(0);
      auto *ptr_type = dyn_cast<llvm::PointerType>(param_type);
      auto *elem_type = ptr_type->getElementType();
      struct_type = dyn_cast<llvm::StructType>(elem_type);
    } else if (struct_type == nullptr) {
      auto struct_name = "struct." + impl_prefix + "_t";
      struct_type = llvm::StructType::create(M.getContext(), struct_name);
    }
    MEMOIR_NULL_CHECK(
        struct_type,
        "Could not find or create the LLVM StructType for " ASSOC_IMPL "!");

    // Create a stack location.
    auto *llvm_alloca = builder.CreateAlloca(struct_type);

    // Initialize the stack location.
    llvm_call =
        builder.CreateCall(function_callee,
                           llvm::ArrayRef<llvm::Value *>({ llvm_alloca }));
  }
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for hashtable alloc");

  println("  --> ", *llvm_call);

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

  this->coalesce(I, *collection);
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitStructAllocInst(StructAllocInst &I) {
  // Get a builder for this instruction.
  MemOIRBuilder builder(I);

  // Get the LLVM StructType for this struct.
  auto &struct_type = I.getStructType();
  auto &type_layout = TC.convert(struct_type);
  auto *llvm_struct_type =
      dyn_cast<llvm::StructType>(&type_layout.get_llvm_type());
  MEMOIR_NULL_CHECK(llvm_struct_type,
                    "TypeLayout did not contain a StructType");
  auto *llvm_ptr_type = llvm::PointerType::get(llvm_struct_type, 0);
  MEMOIR_NULL_CHECK(llvm_ptr_type, "Could not get the LLVM PointerType");

  // Get the in-memory size of the given type.
  auto &data_layout = this->M.getDataLayout();
  auto llvm_struct_size = data_layout.getTypeAllocSize(llvm_struct_type);

  // Get the size of a pointer for the given architecture.
  auto *int_ptr_type = builder.getIntPtrTy(data_layout);

  // Get the constant for the given LLVM struct size.
  auto *llvm_struct_size_constant =
      llvm::ConstantInt::get(int_ptr_type, llvm_struct_size);

  // Get the allocator information for this allocation.
  // TODO: Get this information from attached metadata, we can safely default to
  // malloc though.
  // auto allocator_name = "malloc";

  // Get the allocator function.
  // auto *allocator_function = this->M.getFunction(allocator_name);
  // MEMOIR_NULL_CHECK(allocator_function, "Couldn't get the allocator
  // function!");

  // Create the allocation.
  auto *insertion_point = &I.getCallInst();
  auto *allocation = llvm::CallInst::CreateMalloc(insertion_point,
                                                  int_ptr_type,
                                                  llvm_struct_type,
                                                  llvm_struct_size_constant,
                                                  /* ArraySize = */ nullptr,
                                                  /* MallocF = */ nullptr,
                                                  /* Name = */ "struct.");
  MEMOIR_NULL_CHECK(allocation, "Couldn't create malloc for StructAllocInst");

  // Replace the struct allocation with the new allocation.
  this->coalesce(I, *allocation);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitDeleteCollectionInst(DeleteCollectionInst &I) {
#if COLLECTION_SELECTION
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getCollectionOperand())),
                      "Couldn't determine type of collection");
  if (auto *seq_type = dyn_cast<SequenceType>(&collection_type)) {
    auto &element_type = seq_type->getElementType();

    auto element_code = element_type.get_code();
    auto vector_free_name = *element_code + "_" SEQ_IMPL "__free";

    auto *function = this->M.getFunction(vector_free_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find vector free for ", vector_free_name);
      return;
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *vector_value =
        builder.CreatePointerCast(&I.getCollectionOperand(),
                                  function_type->getParamType(0));
    auto *llvm_call =
        builder.CreateCall(function_callee, llvm::ArrayRef({ vector_value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector read");

    this->markForCleanup(I);
  } else if (auto *assoc_type = dyn_cast<AssocArrayType>(&collection_type)) {
    auto &key_type = assoc_type->getKeyType();
    auto &value_type = assoc_type->getValueType();

    auto key_code = key_type.get_code();
    auto value_code = value_type.get_code();
    auto assoc_free_name =
        *key_code + "_" + *value_code + "_" ASSOC_IMPL "__free";

    auto *function = this->M.getFunction(assoc_free_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find assoc free for ", assoc_free_name);
      return;
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *assoc_value =
        builder.CreatePointerCast(&I.getCollectionOperand(),
                                  function_type->getParamType(0));
    auto *llvm_call =
        builder.CreateCall(function_callee, llvm::ArrayRef({ assoc_value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for assoc read");

    this->markForCleanup(I);
  }
#endif
  return;
}

void SSADestructionVisitor::visitSizeInst(SizeInst &I) {
#if COLLECTION_SELECTION
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getCollectionOperand())),
                      "Couldn't determine type of collection");
  std::string name;
  if (auto *seq_type = dyn_cast<SequenceType>(&collection_type)) {
    auto &element_type = seq_type->getElementType();

    auto element_code = element_type.get_code();
    name = *element_code + "_" SEQ_IMPL "__size";
  } else if (auto *assoc_type = dyn_cast<AssocArrayType>(&collection_type)) {
    auto &key_type = assoc_type->getKeyType();
    auto &value_type = assoc_type->getValueType();

    auto key_code = key_type.get_code();
    auto value_code = value_type.get_code();
    name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__size";
  }

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find size for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *value = builder.CreatePointerCast(&I.getCollectionOperand(),
                                          function_type->getParamType(0));
  auto *llvm_call =
      builder.CreateCall(function_callee, llvm::ArrayRef({ value }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for size");

  this->coalesce(I, *llvm_call);

  this->markForCleanup(I);
#else
#endif
  return;
}

void SSADestructionVisitor::visitIndexReadInst(IndexReadInst &I) {
#if COLLECTION_SELECTION
  // Get a builder.
  MemOIRBuilder builder(I);

  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of read collection");

  auto &element_type = collection_type.getElementType();

  if (auto *sequence_type = dyn_cast<SequenceType>(&collection_type)) {
    // Fetch the vector read function.
    auto element_code = element_type.get_code();
    auto vector_read_name = *element_code + "_" SEQ_IMPL "__read";
    auto *function = this->M.getFunction(vector_read_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      println("Couldn't find vector read name for ", vector_read_name);
      MEMOIR_UNREACHABLE("see above");
    }

    // Construct a call to vector read.
    auto *function_type = function_callee.getFunctionType();
    auto *vector_value =
        builder.CreatePointerCast(&I.getObjectOperand(),
                                  function_type->getParamType(0));
    auto *vector_index =
        builder.CreateZExtOrBitCast(&I.getIndexOfDimension(0),
                                    function_type->getParamType(1));

    auto *llvm_call =
        builder.CreateCall(function_callee,
                           llvm::ArrayRef({ vector_value, vector_index }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector read");

    // Replace the old read value with the new one.
    this->coalesce(I, *llvm_call);

    // Mark the instruction for cleanup because its dead now.
    this->markForCleanup(I);
  } else if (auto *static_tensor_type =
                 dyn_cast<StaticTensorType>(&collection_type)) {
    // Get the type layout for the tensor type.
    auto &type_layout = TC.convert(*static_tensor_type);
    auto &llvm_type = type_layout.get_llvm_type();

    // Get the access information.
    MEMOIR_ASSERT(I.getNumberOfDimensions() == 1,
                  "Only single dimension tensors are currently supported");
    auto &index = I.getIndexOfDimension(0);
    auto &collection_accessed = I.getObjectOperand();

    // Construct a pointer cast for the tensor pointer.
    auto *ptr = builder.CreatePointerCast(&collection_accessed, &llvm_type);

    // Construct a gep for the element.
    auto *gep = builder.CreateInBoundsGEP(
        ptr,
        llvm::ArrayRef<llvm::Value *>({ builder.getInt32(0), &index }));

    // Construct the load of the element.
    auto *load = builder.CreateLoad(gep);

    // Replace old read value with the new one.
    this->coalesce(I, *load);
    // I.getCallInst().replaceAllUsesWith(load);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }
#endif
  return;
}

void SSADestructionVisitor::visitIndexGetInst(IndexGetInst &I) {
#if COLLECTION_SELECTION
  // Get a builder.
  MemOIRBuilder builder(I);

  println(I.getObjectOperand());
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of read collection");

  auto &element_type = collection_type.getElementType();

  if (auto *sequence_type = dyn_cast<SequenceType>(&collection_type)) {
    // Fetch the vector get function.
    auto element_code = element_type.get_code();
    auto vector_read_name = *element_code + "_" SEQ_IMPL "__get";
    auto *function = this->M.getFunction(vector_read_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      println("Couldn't find vector get name for ", vector_read_name);
      MEMOIR_UNREACHABLE("see above");
    }

    // Construct a call to vector get.
    auto *function_type = function_callee.getFunctionType();
    auto *vector_value =
        builder.CreatePointerCast(&I.getObjectOperand(),
                                  function_type->getParamType(0));
    auto *vector_index =
        builder.CreateZExtOrBitCast(&I.getIndexOfDimension(0),
                                    function_type->getParamType(1));

    auto *llvm_call =
        builder.CreateCall(function_callee,
                           llvm::ArrayRef({ vector_value, vector_index }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector get");

    // CAST the resultant to be a memoir Collection pointer, just to make the
    // middle end happy for now.
    auto *collection =
        builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

    // Replace the old read value with the new one.
    this->coalesce(I, *collection);

    // Mark the instruction for cleanup because its dead now.
    this->markForCleanup(I);
  } else if (auto *static_tensor_type =
                 dyn_cast<StaticTensorType>(&collection_type)) {
    // Get the type layout for the tensor type.
    auto &type_layout = TC.convert(*static_tensor_type);
    auto &llvm_type = type_layout.get_llvm_type();

    // Get the access information.
    MEMOIR_ASSERT(I.getNumberOfDimensions() == 1,
                  "Only single dimension tensors are currently supported");
    auto &index = I.getIndexOfDimension(0);
    auto &collection_accessed = I.getObjectOperand();

    // Construct a pointer cast for the tensor pointer.
    auto *ptr =
        builder.CreatePointerCast(&collection_accessed,
                                  llvm::PointerType::get(&llvm_type, 0));

    // Construct a gep for the element.
    auto *gep = builder.CreateInBoundsGEP(
        ptr,
        llvm::ArrayRef<llvm::Value *>({ builder.getInt32(0), &index }));

    // CAST the pointer to match the type of the existing program.
    auto *collection =
        builder.CreatePointerCast(gep, I.getCallInst().getType());

    // Replace old read value with the new one.
    this->coalesce(I, *collection);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }
#endif
  return;
}

void SSADestructionVisitor::visitIndexWriteInst(IndexWriteInst &I) {
#if COLLECTION_SELECTION
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of written collection");

  MemOIRBuilder builder(I);

  if (auto *sequence_type = dyn_cast<SequenceType>(&collection_type)) {
    auto &element_type = collection_type.getElementType();

    auto element_code = element_type.get_code();
    auto vector_write_name = *element_code + "_" SEQ_IMPL "__write";

    auto *function = this->M.getFunction(vector_write_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      println("Couldn't find vector write name for ", vector_write_name);
      MEMOIR_UNREACHABLE("see above");
    }

    auto *function_type = function_callee.getFunctionType();
    auto *vector_value =
        builder.CreatePointerCast(&I.getObjectOperand(),
                                  function_type->getParamType(0));
    auto *vector_index =
        builder.CreateZExtOrBitCast(&I.getIndexOfDimension(0),
                                    function_type->getParamType(1));
    auto *write_value = &I.getValueWritten();

    auto *llvm_call = builder.CreateCall(
        function_callee,
        llvm::ArrayRef({ vector_value, vector_index, write_value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector write");

    // Cleanup the old instruction.
    this->markForCleanup(I);

  } else if (auto *static_tensor_type =
                 dyn_cast<StaticTensorType>(&collection_type)) {
    // Get the type layout for the tensor type.
    auto &type_layout = TC.convert(*static_tensor_type);
    auto &llvm_type = type_layout.get_llvm_type();

    // Get the access information.
    MEMOIR_ASSERT(I.getNumberOfDimensions() == 1,
                  "Only single dimension tensors are currently supported");
    auto &index = I.getIndexOfDimension(0);
    auto &collection_accessed = I.getObjectOperand();
    auto &value_written = I.getValueWritten();

    // Construct a pointer cast for the tensor pointer.
    auto *ptr = builder.CreatePointerCast(&collection_accessed, &llvm_type);

    // Construct a gep for the element.
    auto *gep = builder.CreateInBoundsGEP(
        ptr,
        llvm::ArrayRef<llvm::Value *>({ builder.getInt32(0), &index }));

    // TODO: if this is a bit field, we need to do more bit twiddling.

    // Construct the write to the element.
    builder.CreateStore(&value_written, gep);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }

#endif
  return;
}

void SSADestructionVisitor::visitAssocReadInst(AssocReadInst &I) {
#if COLLECTION_SELECTION
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of read collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__read";

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocRead for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc_value = builder.CreatePointerCast(&I.getObjectOperand(),
                                                function_type->getParamType(0));
  auto *assoc_key =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee,
                         llvm::ArrayRef({ assoc_value, assoc_key }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRead");

  this->coalesce(I, *llvm_call);
  // I.getCallInst().replaceAllUsesWith(llvm_call);

  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssocWriteInst(AssocWriteInst &I) {
#if COLLECTION_SELECTION
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of written collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__write";

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocWrite name for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc_value = builder.CreatePointerCast(&I.getObjectOperand(),
                                                function_type->getParamType(0));
  auto *assoc_index =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));
  auto *write_value = &I.getValueWritten();

  auto *llvm_call = builder.CreateCall(
      function_callee,
      llvm::ArrayRef({ assoc_value, assoc_index, write_value }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocWrite");

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getObjectOperand().getType());

  // Coalesce the result with the corresponding DefPHI.
  for (auto *user : I.getObjectOperand().users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      continue;
    }
    if (!((user_as_inst->getParent() == I.getParent())
          && (user_as_inst > &I.getCallInst()))) {
      continue;
    }
    auto *user_as_memoir = MemOIRInst::get(*user_as_inst);
    if (!user_as_memoir) {
      continue;
    }

    if (auto *def_phi = dyn_cast<DefPHIInst>(user_as_memoir)) {
      this->def_phi_replacements[&def_phi->getCallInst()] = collection;
      break;
    }
  }

  this->markForCleanup(I);

#endif
  return;
}

void SSADestructionVisitor::visitAssocGetInst(AssocGetInst &I) {
#if COLLECTION_SELECTION
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of read collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__get";

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocGet for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc_value = builder.CreatePointerCast(&I.getObjectOperand(),
                                                function_type->getParamType(0));
  auto *assoc_key =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee,
                         llvm::ArrayRef({ assoc_value, assoc_key }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRead");

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

  this->coalesce(I, *collection);

  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssocHasInst(AssocHasInst &I) {
#if COLLECTION_SELECTION
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of has collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__has";

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocHas for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc_value = builder.CreatePointerCast(&I.getObjectOperand(),
                                                function_type->getParamType(0));
  auto *assoc_key =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee,
                         llvm::ArrayRef({ assoc_value, assoc_key }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocHas");

  // I.getCallInst().replaceAllUsesWith(llvm_call);
  this->coalesce(I, *llvm_call);

  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssocRemoveInst(AssocRemoveInst &I) {

#if COLLECTION_SELECTION
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getCollectionOperand())),
                      "Couldn't determine type of written collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__remove";

  auto *function = this->M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocRemove name for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc = builder.CreatePointerCast(&I.getCollectionOperand(),
                                          function_type->getParamType(0));
  auto *assoc_key =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee, llvm::ArrayRef({ assoc, assoc_key }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRemove");

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getCollectionOperand().getType());

  // Coalesce the result with the corresponding DefPHI.
  for (auto *user : I.getCollectionOperand().users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      continue;
    }
    if (!((user_as_inst->getParent() == I.getParent())
          && (user_as_inst > &I.getCallInst()))) {
      continue;
    }
    auto *user_as_memoir = MemOIRInst::get(*user_as_inst);
    if (!user_as_memoir) {
      continue;
    }

    if (auto *def_phi = dyn_cast<DefPHIInst>(user_as_memoir)) {
      this->def_phi_replacements[&def_phi->getCallInst()] = collection;
      break;
    }
  }

  this->markForCleanup(I);

#endif
  return;
}

void SSADestructionVisitor::visitStructReadInst(StructReadInst &I) {
  // Make a builder.
  MemOIRBuilder builder(I);

  // Get the type of the struct being accessed.
  auto &collection_type = I.getCollectionType();
  auto *field_array_type = cast<FieldArrayType>(&collection_type);
  auto &struct_type = field_array_type->getStructType();
  auto &struct_layout = TC.convert(struct_type);
  auto &llvm_type = struct_layout.get_llvm_type();

  // Get the struct being accessed as an LLVM value.
  auto &struct_value = I.getObjectOperand();

  // Get the field information for the access.
  auto field_index = I.getFieldIndex();
  auto field_offset = struct_layout.get_field_offset(field_index);

  // Get the constant for the given field offset.
  auto &data_layout = this->M.getDataLayout();
  auto *int_ptr_type = builder.getIntPtrTy(data_layout);

  // Construct a pointer cast to the LLVM struct type.
  auto *ptr = builder.CreatePointerCast(&struct_value,
                                        llvm::PointerType::get(&llvm_type, 0));

  // Construct the GEP for the field.
  auto *gep = builder.CreateStructGEP(ptr, field_offset);

  // Construct the load.
  llvm::Value *load = builder.CreateLoad(gep, /* isVolatile = */ false);

  // If the field is a bit field, pay the bit twiddler their due.
  if (struct_layout.is_bit_field(field_index)) {
    // Fetch the bit field range.
    auto bit_field_range = *(struct_layout.get_bit_field_range(field_index));
    auto bit_field_start = bit_field_range.first;
    auto bit_field_end = bit_field_range.second;
    auto bit_field_width = bit_field_end - bit_field_start;

    // If the field is signed, we need to become the king bit twiddler.
    bool is_signed = false;
    auto &field_type = struct_type.getFieldType(field_index);

    if (auto *int_field_type = dyn_cast<IntegerType>(&field_type)) {
      if (int_field_type->isSigned()) {
        is_signed = true;

        // Get the size of the containing bit field.
        auto *llvm_field_type = I.getCallInst().getType();
        auto *llvm_int_field_type = cast<llvm::IntegerType>(llvm_field_type);
        auto llvm_field_width = int_field_type->getBitWidth();

        // SHIFT the bit field over to the top bits.
        auto left_shift_distance = llvm_field_width - bit_field_end;
        load = builder.CreateShl(load, left_shift_distance);

        // ARITHMETIC SHIFT the bit field over the low bits.
        auto right_shift_distance = llvm_field_width - bit_field_width;
        load = builder.CreateAShr(load, right_shift_distance);
      }
    }

    // SHIFT the values over.
    if (!is_signed) {
      load = builder.CreateLShr(load, bit_field_start);
    }

    // MASK the value.
    uint64_t mask = 0;
    for (int i = 0; i < bit_field_width; ++i) {
      mask |= 1 << i;
    }
    load = builder.CreateAnd(load, mask);

    // BITCAST the value, if needed.
    load = builder.CreateIntCast(load, I.getCallInst().getType(), is_signed);
  }

  // Coalesce and return.
  this->coalesce(I, *load);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitStructWriteInst(StructWriteInst &I) {
  MemOIRBuilder builder(I);

  // Get the type of the struct being accessed.
  auto &collection_type = I.getCollectionType();
  auto *field_array_type = cast<FieldArrayType>(&collection_type);
  auto &struct_type = field_array_type->getStructType();
  auto &struct_layout = TC.convert(struct_type);
  auto &llvm_type = struct_layout.get_llvm_type();

  // Get the struct being accessed as an LLVM value.
  auto &struct_value = I.getObjectOperand();

  // Get the field information for the access.
  auto field_index = I.getFieldIndex();
  auto field_offset = struct_layout.get_field_offset(field_index);

  // Get the constant for the given field offset.
  auto &data_layout = this->M.getDataLayout();
  auto *int_ptr_type = builder.getIntPtrTy(data_layout);

  // Construct a pointer cast to the LLVM struct type.
  auto *ptr = builder.CreatePointerCast(&struct_value,
                                        llvm::PointerType::get(&llvm_type, 0));

  // Construct the GEP for the field.
  auto *gep = builder.CreateStructGEP(ptr, field_offset);

  // Get the value being written.
  auto *value_written = &I.getValueWritten();

  // If the field is a bit field, load the resident value, perform the requisite
  // bit twiddling, and then store the value.
  if (struct_layout.is_bit_field(field_index)) {
    llvm::Value *load = builder.CreateLoad(gep);

    // Fetch the bit field range.
    auto bit_field_range = *(struct_layout.get_bit_field_range(field_index));
    auto bit_field_start = bit_field_range.first;
    auto bit_field_end = bit_field_range.second;

    // BITCAST the value, if needed.
    value_written =
        builder.CreateIntCast(value_written, load->getType(), false);

    // MASK the bits.
    uint64_t mask = 0;
    auto bit_field_width = bit_field_end - bit_field_start;
    for (unsigned i = 0; i < bit_field_width; ++i) {
      mask |= 1 << i;
    }
    value_written = builder.CreateAnd(value_written, mask);

    // SHIFT the bits into the correct position.
    if (bit_field_start != 0) {
      value_written = builder.CreateShl(value_written, bit_field_start);
    }

    // MASK out the bits from the loaded value.
    mask = 0;
    for (auto i = bit_field_start; i < bit_field_end; ++i) {
      mask |= 1 << i;
    }

    load = builder.CreateAnd(load, ~mask);

    // OR the bits with those currently in the memory location.
    value_written = builder.CreateOr(value_written, load);
  }

  // Construct the load.
  auto &store = MEMOIR_SANITIZE(
      builder.CreateStore(value_written, gep, /* isVolatile = */ false),
      "Failed to create the LLVM store for StructWriteInst");

  // Coalesce and return.
  this->coalesce(I, store);

  // I.getCallInst().replaceAllUsesWith(&store);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitStructGetInst(StructGetInst &I) {

  return;
}

void SSADestructionVisitor::visitUsePHIInst(UsePHIInst &I) {

  auto &used_collection = I.getUsedCollectionOperand();
  auto &collection = I.getCollectionValue();

  this->coalesce(collection, used_collection);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitDefPHIInst(DefPHIInst &I) {

  auto &defined_collection = I.getDefinedCollectionOperand();
  auto &collection = I.getCollectionValue();

  auto found_replacement = this->def_phi_replacements.find(&I.getCallInst());
  if (found_replacement != this->def_phi_replacements.end()) {
    this->coalesce(collection, *found_replacement->second);
  } else {
    this->coalesce(collection, defined_collection);
  }

  this->markForCleanup(I);

  return;
}

static void slice_to_view(SliceInst &I) {

  auto &call_inst = I.getCallInst();
  auto *view_func = FunctionNames::get_memoir_function(*call_inst.getModule(),
                                                       MemOIR_Func::VIEW);
  MEMOIR_NULL_CHECK(view_func, "Could not find the memoir view function");
  call_inst.setCalledFunction(view_func);
  return;
}

void SSADestructionVisitor::visitSliceInst(SliceInst &I) {

  auto &collection = I.getCollectionOperand();
  auto &slice = I.getSliceAsValue();

  // If the collection is dead immediately following,
  // then we can replace this slice with a view.
  if (!this->LA->is_live(collection, I)) {
    slice_to_view(I);
    return;
  }

  // If the slice is disjoint from the live slice range of the collection.
  bool is_disjoint = true;
  auto &slice_begin = I.getBeginIndex();
  auto &slice_end = I.getEndIndex();
  set<SliceInst *> slice_users = {};
  for (auto *user : collection.users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      // This is an overly conservative check.
      is_disjoint = false;
      break;
    }

    // Check that the user is dominated by this instruction.
    if (!this->DT->dominates(&I.getCallInst(), user_as_inst)) {
      break;
    }

    auto *user_as_memoir = MemOIRInst::get(*user_as_inst);
    if (!user_as_memoir) {
      // Also overly conservative, we _can_ handle PHIs.
      is_disjoint = false;
      break;
    }

    if (auto *user_as_slice = dyn_cast<SliceInst>(user_as_memoir)) {
      // We will check all slice users for non-overlapping index spaces, _if_
      // there are no other users.
      slice_users.insert(user_as_slice);
      continue;
    }

    if (auto *user_as_access = dyn_cast<AccessInst>(user_as_memoir)) {
      // TODO: check the interval range of the index.
    }

    is_disjoint = false;
    break;
  }

  if (!is_disjoint) {
    return;
  }

  slice_users.erase(&I);

  // Check the slice users to see if they are non-overlapping.
  if (!slice_users.empty()) {
    auto &slice_begin = I.getBeginIndex();
    auto &slice_end = I.getEndIndex();
    set<SliceInst *> visited = {};
    list<llvm::Value *> limits = { &slice_begin, &slice_end };
    debugln("check non-overlapping");
    while (visited.size() < slice_users.size()) {
      bool found_new_limit = false;
      for (auto *user_as_slice : slice_users) {
        if (visited.find(user_as_slice) != visited.end()) {
          continue;
        }

        auto &user_slice_begin = user_as_slice->getBeginIndex();
        auto &user_slice_end = user_as_slice->getEndIndex();

        debugln("lower limit: ", *limits.front());
        debugln("upper limit: ", *limits.back());
        debugln(" user begin: ", user_slice_begin);
        debugln(" user   end: ", user_slice_end);

        // Check if this slice range is overlapping.
        if (&user_slice_begin == limits.back()) {
          visited.insert(user_as_slice);
          limits.push_back(&user_slice_end);
          found_new_limit = true;
        } else if (&user_slice_end == limits.front()) {
          visited.insert(user_as_slice);
          limits.push_front(&user_slice_begin);
          found_new_limit = true;
        }
      }

      // If we found a new limit, continue working.
      if (!found_new_limit) {
        break;
      }
    }

    // Otherwise, we need to bring out the big guns and check for relations.
    auto *slice_begin_expr = this->VN->get(slice_begin);
    auto *slice_end_expr = this->VN->get(slice_end);
    ValueExpression *new_lower_limit = nullptr;
    ValueExpression *new_upper_limit = nullptr;
    for (auto *user_as_slice : slice_users) {
      if (visited.find(user_as_slice) != visited.end()) {
        continue;
      }

      // Check if this slice range is non-overlapping, with an offset.
      auto *user_slice_begin_expr =
          this->VN->get(user_as_slice->getBeginIndex());
      MEMOIR_NULL_CHECK(user_slice_begin_expr,
                        "Error making value expression for begin index");
      auto *user_slice_end_expr = this->VN->get(user_as_slice->getEndIndex());
      MEMOIR_NULL_CHECK(user_slice_end_expr,
                        "Error making value expression for end index");

      debugln("Checking left");
      debugln("  ", user_as_slice->getBeginIndex());
      debugln("  ", slice_begin);
      auto check_left = *user_slice_end_expr < *slice_begin_expr;

      debugln("Checking right");
      debugln("  ", user_as_slice->getEndIndex());
      debugln("  ", slice_end);
      auto check_right = *user_slice_begin_expr > *slice_end_expr;

      if (check_left || check_right) {
        continue;
      } else {
        warnln("Big guns failed,"
               " open an issue if this shouldn't have happened.");
        is_disjoint = false;
      }
    }
  }

  if (is_disjoint) {
    slice_to_view(I);
  }

  return;
}

void SSADestructionVisitor::visitJoinInst(JoinInst &I) {

  auto &collection = I.getCollectionAsValue();
  auto num_joined = I.getNumberOfJoins();

  // For each join operand, if it is dead after the join, we can coallesce this
  // join with it.
  bool all_dead = true;
  for (auto join_idx = 0; join_idx < num_joined; join_idx++) {
    auto &joined_use = I.getJoinedOperandAsUse(join_idx);
    auto &joined_collection =
        MEMOIR_SANITIZE(joined_use.get(), "Use by join is NULL!");
    if (this->LA->is_live(joined_collection, I)) {
      all_dead = false;

      debugln("Live after join!");
      debugln("        ", joined_collection);
      debugln("  after ", I);

      break;
    }
  }

  if (!all_dead) {
    infoln("Not all incoming values are dead after a join.");
    infoln(" |-> ", I);
    return;
  }

  // If all operands of the join are views of the same collection:
  //  - If views are in order, coallesce resultant and the viewed collection.
  //  - Otherwise, determine if the size is the same after the join:
  //     - If the size is the same, convert to a swap.
  //     - Otherwise, convert to a remove.
  vector<size_t> view_indices = {};
  view_indices.reserve(num_joined);
  vector<ViewInst *> views = {};
  views.reserve(num_joined);
  bool all_views = true;
  llvm::Value *base_collection = nullptr;
  for (auto join_idx = 0; join_idx < num_joined; join_idx++) {
    auto &joined_use = I.getJoinedOperandAsUse(join_idx);
    auto &joined_collection =
        MEMOIR_SANITIZE(joined_use.get(), "Use by join is NULL!");

    auto *joined_as_inst = dyn_cast<llvm::Instruction>(&joined_collection);
    if (!joined_as_inst) {
      all_views = false;
      break;
    }

    auto *joined_collection_as_memoir = MemOIRInst::get(*joined_as_inst);
    llvm::Value *used_collection;
    if (auto *view_inst =
            dyn_cast_or_null<ViewInst>(joined_collection_as_memoir)) {
      auto &viewed_collection = view_inst->getCollectionOperand();
      used_collection = &viewed_collection;
      views.push_back(view_inst);
      view_indices.push_back(join_idx);
    } else {
      infoln("Join uses non-view");
      infoln(" |-> ", I);
      all_views = false;
      continue;
    }

    // See if we are still using the same base collection.
    if (base_collection == nullptr) {
      base_collection = used_collection;
      continue;
    } else if (base_collection == used_collection) {
      continue;
    } else {
      base_collection = nullptr;
      break;
    }
  }

  if (base_collection != nullptr && all_views) {
    infoln("Join uses a single base collection");
    infoln(" |-> ", I);

    // Determine if the size is the same.
    set<ViewInst *> visited = {};
    list<size_t> view_order = {};
    list<pair<llvm::Value *, llvm::Value *>> ranges_to_remove = {};
    llvm::Value *lower_limit = nullptr;
    llvm::Value *upper_limit = nullptr;
    bool swappable = true;
    while (visited.size() != views.size()) {
      bool new_limit = false;
      for (auto view_idx = 0; view_idx < views.size(); view_idx++) {
        auto *view = views[view_idx];
        auto &begin_index = view->getBeginIndex();
        auto &end_index = view->getEndIndex();
        if (lower_limit == nullptr) {
          lower_limit = &begin_index;
          upper_limit = &end_index;
          visited.insert(view);
          view_order.push_back(view_idx);
          new_limit = true;
          continue;
        } else if (lower_limit == &end_index) {
          lower_limit = &begin_index;
          visited.insert(view);
          view_order.push_front(view_idx);
          new_limit = true;
          continue;
        } else if (upper_limit == &begin_index) {
          upper_limit = &end_index;
          visited.insert(view);
          view_order.push_back(view_idx);
          new_limit = true;
          continue;
        }
      }

      if (new_limit) {
        continue;
      }

      // Determine the ranges that must be removed.
      auto *lower_limit_expr = this->VN->get(*lower_limit);
      auto *upper_limit_expr = this->VN->get(*upper_limit);
      ValueExpression *new_lower_limit_expr = nullptr;
      ViewInst *lower_limit_view = nullptr;
      ValueExpression *new_upper_limit_expr = nullptr;
      ViewInst *upper_limit_view = nullptr;
      for (auto view_idx = 0; view_idx < views.size(); view_idx++) {
        auto *view = views[view_idx];
        if (visited.find(view) != visited.end()) {
          continue;
        }

        // Find the nearest
        auto &view_begin_expr =
            MEMOIR_SANITIZE(this->VN->get(view->getBeginIndex()),
                            "Error making value expression for begin index");
        auto &view_end_expr =
            MEMOIR_SANITIZE(this->VN->get(view->getEndIndex()),
                            "Error making value expression for end index");

        auto check_left = view_end_expr < *lower_limit_expr;
        auto check_right = view_begin_expr > *upper_limit_expr;

        if (check_left) {
          // Check if this is the new lower bound.
          if (new_lower_limit_expr == nullptr) {
            new_lower_limit_expr = &view_end_expr;
            lower_limit_view = view;
            new_limit = true;
          } else if (view_end_expr < *new_lower_limit_expr) {
            new_lower_limit_expr = &view_end_expr;
            lower_limit_view = view;
            new_limit = true;
          }
          continue;
        } else if (check_right) {
          // Check if this is the new upper bound.
          if (new_upper_limit_expr == nullptr) {
            new_upper_limit_expr = &view_begin_expr;
            upper_limit_view = view;
            new_limit = true;
          } else if (view_begin_expr > *new_upper_limit_expr) {
            new_upper_limit_expr = &view_begin_expr;
            upper_limit_view = view;
            new_limit = true;
          }
          continue;
        }
      }

      // If we found a new limit, update the lower and/or upper limit and mark
      // the range for removal.
      if (new_limit) {
        if (lower_limit_view != nullptr) {
          auto &new_lower_limit = lower_limit_view->getEndIndex();
          ranges_to_remove.push_front(make_pair(&new_lower_limit, lower_limit));
          lower_limit = &new_lower_limit;
        }

        if (upper_limit_view != nullptr) {
          auto &new_upper_limit = upper_limit_view->getBeginIndex();
          ranges_to_remove.push_back(make_pair(upper_limit, &new_upper_limit));
          upper_limit = &new_upper_limit;
        }

        // Keep on chugging.
        continue;
      }

      println("!!!!");
      println("The join: ", I);
      MEMOIR_UNREACHABLE(
          "We shouldn't be here, couldnt' find out what to swap/remove forthe above join!");
    }

    if (swappable) {
      infoln("Convert the join to a swap.");
      infoln(" |-> ", I);
      infoln(" |-> Original order:");
      for (auto view_idx : view_order) {
        infoln(" | |-> ", *views[view_idx]);
      }
      infoln(" |-> New order:");
      for (auto view : views) {
        infoln(" | |-> ", *view);
      }

#if COLLECTION_SELECTION
      auto &collection_type =
          MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                              TypeAnalysis::analyze(I.getCallInst())),
                          "Couldn't determine type of join");

      auto &element_type = collection_type.getElementType();

      // TODO: make this more extensible.
      auto element_code = element_type.get_code();
      auto vector_name = *element_code + "_" SEQ_IMPL "__swap";

      auto *function = this->M.getFunction(vector_name);
      auto function_callee = FunctionCallee(function);
      if (function == nullptr) {
        println("Couldn't find vector swap range for ", vector_name);
        MEMOIR_UNREACHABLE("see above");
      }

      MemOIRBuilder builder(I);

      auto *function_type = function_callee.getFunctionType();
      auto *vector_value =
          builder.CreatePointerCast(base_collection,
                                    function_type->getParamType(0));
#endif

      auto new_idx = 0;
      auto i = 0;
      for (auto it = view_order.begin(); it != view_order.end(); ++it, i++) {
        auto orig_idx = *it;
        if (orig_idx != new_idx) {
          infoln(" |-> Swap ", orig_idx, " <=> ", new_idx);

          // Check that these views are the same size.
          auto &from_begin = views[orig_idx]->getBeginIndex();
          auto *from_begin_expr = this->VN->get(from_begin);
          auto &from_end = views[orig_idx]->getEndIndex();
          auto *from_end_expr = this->VN->get(from_end);
          auto &to_begin = views[new_idx]->getBeginIndex();
          auto *to_begin_expr = this->VN->get(to_begin);
          auto &to_end = views[new_idx]->getEndIndex();
          auto *to_end_expr = this->VN->get(to_end);

          // Create the expression for the size calculation.
          auto *from_size_expr = new BasicExpression(
              llvm::Instruction::Sub,
              vector<ValueExpression *>({ from_end_expr, from_begin_expr }));
          auto *to_size_expr = new BasicExpression(
              llvm::Instruction::Sub,
              vector<ValueExpression *>({ to_end_expr, to_begin_expr }));

          // If they are the same size, swap them.
          if (*from_size_expr == *to_size_expr) {
            // Create the swap.
            MemOIRBuilder builder(I);
#if COLLECTION_SELECTION
            vector_value = builder.CreateCall(
                function_callee,
                llvm::ArrayRef(
                    { vector_value, &from_begin, &from_end, &to_begin }));
            MEMOIR_NULL_CHECK(
                vector_value,
                "Could not create the call for vector remove range");
#else
            builder.CreateSeqSwapInst(base_collection,
                                      &from_begin,
                                      &from_end,
                                      &to_begin);
#endif

            // Swap the operands.
            auto &from_use = I.getJoinedOperandAsUse(orig_idx);
            auto &to_use = I.getJoinedOperandAsUse(new_idx);
            from_use.swap(to_use);

            // NOTE: we don't need to update other users here, as these views
            // were already checked to be dead following.

            // Update the view order post-swap.
            auto r = view_order.size() - 1;
            for (auto rit = view_order.rbegin(); r > i; ++rit, r--) {
              if (*rit == new_idx) {
                std::swap(*(views.begin() + i), *(views.begin() + r));
                std::swap(*it, *rit);
                break;
              }
            }
          } else {
            MEMOIR_UNREACHABLE("Size of from and to for swap are not the same! "
                               "Someone must have changed the program.");
          }
        }
        new_idx++;
      }

      // Replace the join with the base collection that has been swapped.
      this->coalesce(I, *base_collection);
      this->markForCleanup(I);
    }

#if COLLECTION_SELECTION
    auto &collection_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                            TypeAnalysis::analyze(I.getCallInst())),
                        "Couldn't determine type of join");

    auto &element_type = collection_type.getElementType();

    // TODO: make this more extensible.
    auto element_code = element_type.get_code();
    auto vector_remove_range_name =
        *element_code + "_" SEQ_IMPL "__remove_range";

    auto *function = this->M.getFunction(vector_remove_range_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      println("Couldn't find vector remove range for ",
              vector_remove_range_name);
      MEMOIR_UNREACHABLE("see above");
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *vector_value =
        builder.CreatePointerCast(base_collection,
                                  function_type->getParamType(0));
#endif

    // Now to remove any ranges that were marked.
    for (auto range : ranges_to_remove) {
      auto *begin = range.first;
      auto *end = range.second;
      MemOIRBuilder builder(I);
      // TODO: check if this range is alive as a view, if it is we need to
      // convert it to a slice.
#if COLLECTION_SELECTION
      vector_value =
          builder.CreateCall(function_callee,
                             llvm::ArrayRef({ vector_value, begin, end }));
      MEMOIR_NULL_CHECK(vector_value,
                        "Could not create the call for vector remove range");
#else
      builder.CreateSeqRemoveInst(base_collection, begin, end);
#endif
    }

#if COLLECTION_SELECTION
    auto *collection =
        builder.CreatePointerCast(vector_value, I.getCallInst().getType());
    this->coalesce(I, *collection);
#endif

  } else if (base_collection != nullptr) /* not all_views */ {
    // Determine if the views are in order.
    // If they are, we do an insert.
    llvm::Value *lower_limit, *upper_limit;
    bool base_is_ordered = true;
    auto idx_it = view_indices.begin();
    for (auto view_it = views.begin(); view_it != views.end(); ++view_it) {
      auto *view = *view_it;
      auto cur_idx = *idx_it;
      if (++idx_it == view_indices.end()) {
        break;
      }
      auto next_idx = *idx_it;
      if (next_idx > cur_idx + 1) {
        auto *next_view = *(++view_it);
        --view_it; // revert the last iteration.

        auto &cur_view_end = view->getEndIndex();
        auto &next_view_begin = next_view->getBeginIndex();

        // If the views at this insertion point are the same, then we need to
        // insert.
        if (&cur_view_end == &next_view_begin) {
          MemOIRBuilder builder(I);

#if COLLECTION_SELECTION
          auto &collection_type =
              MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                                  TypeAnalysis::analyze(I.getCallInst())),
                              "Couldn't determine type of join");

          auto &element_type = collection_type.getElementType();

          auto element_code = element_type.get_code();
          auto vector_remove_range_name =
              *element_code + "_" SEQ_IMPL "__insert";

          auto *function = this->M.getFunction(vector_remove_range_name);
          if (function == nullptr) {
            println("Couldn't find vector insert for ",
                    vector_remove_range_name);
            MEMOIR_UNREACHABLE("see above");
          }
          auto function_callee = FunctionCallee(function);

          auto *function_type = function_callee.getFunctionType();
          auto *vector =
              builder.CreatePointerCast(base_collection,
                                        function_type->getParamType(0));
#endif

          llvm::Value *insertion_point = nullptr;
          llvm::Value *last_inserted_collection = nullptr;
          for (auto idx = cur_idx + 1; idx < next_idx; idx++) {
            // Get the collection to insert.
            auto &collection_to_insert = I.getJoinedOperand(idx);

            // Track the insertion point.
            if (insertion_point == nullptr) {
              insertion_point = &cur_view_end;
            } else {
              // TODO: improve me.
#if COLLECTION_SELECTION
              auto vector_size_name = *element_code + "_" SEQ_IMPL "__size";

              auto *size_function = this->M.getFunction(vector_size_name);
              if (size_function == nullptr) {
                println("Couldn't find vector size for ", vector_size_name);
                MEMOIR_UNREACHABLE("see above");
              }
              auto size_function_callee = FunctionCallee(size_function);

              auto *last_size = builder.CreateCall(
                  size_function_callee,
                  llvm::ArrayRef({ last_inserted_collection }));
#else
              auto *last_size =
                  &builder.CreateSizeInst(last_inserted_collection)
                       ->getCallInst();
#endif
              insertion_point = builder.CreateAdd(insertion_point, last_size);
            }

            // TODO: See if the collection is a single element, if so use its
            // specialized insert method. Tricky part is finding the value to
            // insert.
            // auto &inserted_collection = I.getJoinedCollection(idx);
            // if (auto *inserted_alloc =
            //         dyn_cast<BaseCollection>(inserted_collection)) {
            //   auto *collection_alloc = inserted_alloc->getAllocation();
            //   auto *sequence_alloc =
            //       dyn_cast_or_null<SequenceAllocInst>(collection_alloc);
            //   MEMOIR_NULL_CHECK(sequence_alloc,
            //                     "Allocation being inserted is not a
            //                     sequence!");
            //   auto &sequence_size = sequence_alloc->getSizeOperand();
            //   if (auto *size_as_const_int =
            //           dyn_cast<llvm::ConstantInt *>(&sequence_size)) {
            //     if (size_as_const_int->isOne()) {
            //       auto *element_type = sequence_alloc->getElementType();
            //       builder.CreateSeqInsertInst(base_collection,
            //       insertion_point, value_to_insert
            //     }
            //   }
            // }

            // Create the insert instruction.
#if COLLECTION_SELECTION
            auto *vector_to_insert =
                builder.CreatePointerCast(&collection_to_insert,
                                          function_type->getParamType(2));
            vector = builder.CreateCall(
                function_callee,
                llvm::ArrayRef({ vector, insertion_point, vector_to_insert }));
            last_inserted_collection = vector;
#else
            builder.CreateSeqInsertSeqInst(base_collection,
                                           insertion_point,
                                           &collection_to_insert);
            last_inserted_collection = &collection_to_insert;
#endif
          }

#if COLLECTION_SELECTION
          vector = builder.CreatePointerCast(vector, I.getCallInst().getType());
          this->coalesce(I, *vector);
#else
          this->coalesce(I, *base_collection);
#endif
          this->markForCleanup(I);
        }
      }
    }
  }

  // Otherwise, some operands are views from a different collection:
  //  - If the size is the same, convert to a swap.
  //  - Otherwise, convert to an append.
  else /* not base_collection, not all_views*/ {

    // If there are no views being used, we simply have an append.
    if (views.empty()) {
      MemOIRBuilder builder(I);

      auto &first_joined_operand = I.getJoinedOperand(0);

#if COLLECTION_SELECTION
      auto &collection_type =
          MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                              TypeAnalysis::analyze(I.getCallInst())),
                          "Couldn't determine type of join");
      auto &element_type = collection_type.getElementType();

      auto element_code = element_type.get_code();

      auto insert_name = *element_code + "_" SEQ_IMPL "__insert";
      auto size_name = *element_code + "_" SEQ_IMPL "__size";

      auto *insert_function = this->M.getFunction(insert_name);
      if (insert_function == nullptr) {
        println("Couldn't find vector insert for ", insert_name);
        MEMOIR_UNREACHABLE("see above");
      }
      auto insert_function_callee = FunctionCallee(insert_function);
      auto *insert_function_type = insert_function_callee.getFunctionType();

      auto *size_function = this->M.getFunction(size_name);
      if (size_function == nullptr) {
        println("Couldn't find vector size for ", size_name);
        MEMOIR_UNREACHABLE("see above");
      }
      auto size_function_callee = FunctionCallee(size_function);
      auto *size_function_type = size_function_callee.getFunctionType();

      auto *vector =
          builder.CreatePointerCast(&first_joined_operand,
                                    insert_function_type->getParamType(0));

      auto *insertion_point =
          builder.CreateCall(size_function_callee, llvm::ArrayRef({ vector }));
#endif

      for (auto join_idx = 1; join_idx < num_joined; join_idx++) {
        auto &joined_operand = I.getJoinedOperand(join_idx);
#if COLLECTION_SELECTION
        auto *insertion_point = builder.CreateCall(size_function_callee,
                                                   llvm::ArrayRef({ vector }));

        auto *vector_to_insert =
            builder.CreatePointerCast(&joined_operand,
                                      insert_function_type->getParamType(2));
        vector = builder.CreateCall(
            insert_function_callee,
            llvm::ArrayRef<llvm::Value *>(
                { vector, insertion_point, vector_to_insert }));
#else
        builder.CreateSeqAppendInst(&first_joined_operand, &joined_operand);
#endif
      }

      this->coalesce(I, first_joined_operand);
      this->markForCleanup(I);
    }
  }

  return;
}

void SSADestructionVisitor::visitAssertCollectionTypeInst(
    AssertCollectionTypeInst &I) {
#if COLLECTION_SELECTION
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssertStructTypeInst(AssertStructTypeInst &I) {
#if COLLECTION_SELECTION
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitReturnTypeInst(ReturnTypeInst &I) {
#if COLLECTION_SELECTION
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitTypeInst(TypeInst &I) {
#if COLLECTION_SELECTION
  // Cleanup the global variable store/load if it exists.
  for (auto &use : I.getCallInst().uses()) {
    // Get the user.
    auto *user = use.getUser();
    auto *user_as_inst = dyn_cast_or_null<llvm::Instruction>(user);
    if (!user_as_inst) {
      continue;
    }

    // If the user is a store to a global variable, mark it for cleanup.
    if (auto *user_as_store = dyn_cast<llvm::StoreInst>(user_as_inst)) {
      // Get the global variable referenced being stored to.
      llvm::Value *global_ptr = nullptr;
      auto *ptr = user_as_store->getPointerOperand();
      if (auto *ptr_as_global = dyn_cast<llvm::GlobalVariable>(ptr)) {
        global_ptr = ptr_as_global;
      } else if (auto *ptr_as_gep = dyn_cast<llvm::GetElementPtrInst>(ptr)) {
        global_ptr = ptr_as_gep->getPointerOperand();
      } else if (auto *ptr_as_const_gep = dyn_cast<llvm::ConstantExpr>(ptr)) {
        if (ptr_as_const_gep->isGEPWithNoNotionalOverIndexing()) {
          global_ptr = ptr_as_const_gep->getOperand(0);
        }
      }

      if (global_ptr == nullptr) {
        warnln("memoir type is stored to a non-global variable!");
      }

      // Mark all users of the global variable for cleanup.
      for (auto &ptr_use : ptr->uses()) {
        // Get the user.
        auto *ptr_user = ptr_use.getUser();
        auto *ptr_user_as_inst = dyn_cast_or_null<llvm::Instruction>(ptr_user);
        if (!ptr_user_as_inst) {
          continue;
        }

        // Mark the user for cleanup.
        this->markForCleanup(*ptr_user_as_inst);
      }
    }
  }

  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::cleanup() {
  for (auto *inst : instructions_to_delete) {
    infoln(*inst);
    inst->eraseFromParent();
  }
}

void SSADestructionVisitor::coalesce(MemOIRInst &I, llvm::Value &replacement) {
  this->coalesce(I.getCallInst(), replacement);
}

void SSADestructionVisitor::coalesce(llvm::Value &V, llvm::Value &replacement) {
  infoln("Coalesce:");
  infoln("  ", V);
  infoln("  ", replacement);
  this->coalesced_values[&V] = &replacement;
}

llvm::Value *SSADestructionVisitor::find_replacement(llvm::Value *value) {
  auto *replacement_value = value;
  auto found = this->replaced_values.find(value);
  while (found != this->replaced_values.end()) {
    replacement_value = found->second;
    found = this->replaced_values.find(replacement_value);
  }
  return replacement_value;
}

void SSADestructionVisitor::do_coalesce(llvm::Value &V) {
  auto found_coalesce = this->coalesced_values.find(&V);
  if (found_coalesce == this->coalesced_values.end()) {
    return;
  }

  auto *replacement = this->find_replacement(found_coalesce->second);

  infoln("Coalescing:");
  infoln("  ", V);
  infoln("  ", *replacement);

  V.replaceAllUsesWith(replacement);

  // Update the types of users.
  // for (auto *user : V.users()) {
  //   if (auto *user_as_phi = dyn_cast<llvm::PHINode>(user)) {
  //     if (user_as_phi->getType() != V.getType()) {
  //       user_as_phi->mutateType(V.getType());
  //     }
  //   }
  // }

  this->replaced_values[&V] = replacement;
}

void SSADestructionVisitor::markForCleanup(MemOIRInst &I) {
  this->markForCleanup(I.getCallInst());
}

void SSADestructionVisitor::markForCleanup(llvm::Instruction &I) {
  this->instructions_to_delete.insert(&I);
}

} // namespace llvm::memoir
