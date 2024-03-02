#include "memoir/utility/FunctionNames.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/lowering/TypeLayout.hpp"

#include "SSADestruction.hpp"

#define ASSOC_IMPL "stl_unordered_map"
#define SEQ_IMPL "stl_vector"

namespace llvm::memoir {

SSADestructionVisitor::SSADestructionVisitor(llvm::Module &M,
                                             SSADestructionStats *stats,
                                             bool enable_collection_lowering)
  : M(M),
    TC(M.getContext()),
    stats(stats),
    enable_collection_lowering(enable_collection_lowering) {
  // Do nothing.
}

void SSADestructionVisitor::setAnalyses(llvm::DominatorTree &DT,
                                        LivenessAnalysis &LA) {
  this->DT = &DT;
  this->LA = &LA;

  return;
}

void SSADestructionVisitor::visitInstruction(llvm::Instruction &I) {
  return;
}

void SSADestructionVisitor::visitSequenceAllocInst(SequenceAllocInst &I) {
  if (this->enable_collection_lowering) {
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
      MEMOIR_NULL_CHECK(llvm_call,
                        "Could not create the call for vector alloc");
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

    auto *return_type = I.getCallInst().getType();
    if (!return_type->isVoidTy()) {
      auto *collection =
          builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

      this->coalesce(I, *collection);
    }

    this->markForCleanup(I);
  } else {
    // Do nothing.
  }
  return;
}

void SSADestructionVisitor::visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
  if (this->enable_collection_lowering) {
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
      debugln("value type size = ", llvm_value_size);
      if (auto *arr_type = dyn_cast<llvm::ArrayType>(&llvm_value_type)) {
        debugln("  element size = ",
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
    MEMOIR_NULL_CHECK(llvm_call,
                      "Could not create the call for hashtable alloc");

    auto *collection =
        builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

    this->coalesce(I, *collection);
    this->markForCleanup(I);
  } else {
  }
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

  auto *alloc_ptr =
      builder.CreatePointerCast(allocation, I.getCallInst().getType());

  // Replace the struct allocation with the new allocation.
  this->coalesce(I, *alloc_ptr);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitDeleteCollectionInst(DeleteCollectionInst &I) {
  if (this->enable_collection_lowering) {
    auto &collection_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                            TypeAnalysis::analyze(I.getDeletedCollection())),
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
          builder.CreatePointerCast(&I.getDeletedCollection(),
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
          builder.CreatePointerCast(&I.getDeletedCollection(),
                                    function_type->getParamType(0));
      auto *llvm_call =
          builder.CreateCall(function_callee, llvm::ArrayRef({ assoc_value }));
      MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for assoc read");

      this->markForCleanup(I);
    }
  } else {
    // Do nothing.
  }
  return;
}

void SSADestructionVisitor::visitSizeInst(SizeInst &I) {
  if (this->enable_collection_lowering) {
    auto &collection_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                            TypeAnalysis::analyze(I.getCollection())),
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
    auto *value = builder.CreatePointerCast(&I.getCollection(),
                                            function_type->getParamType(0));
    auto *llvm_call =
        builder.CreateCall(function_callee, llvm::ArrayRef({ value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for size");

    this->coalesce(I, *llvm_call);

    this->markForCleanup(I);
  } else {
    // Do nothing.
  }
  return;
}

static llvm::Value &contextualize_end(EndInst &end_inst,
                                      llvm::Use &use,
                                      InsertInst &insert_inst) {
  MemOIRBuilder builder(insert_inst);

  auto *size_inst = builder.CreateSizeInst(&insert_inst.getBaseCollection());

  MEMOIR_NULL_CHECK(size_inst,
                    "Could not contextualize EndInst for InsertInst!");

  use.set(&size_inst->getCallInst());

  return size_inst->getCallInst();
}

static llvm::Value &contextualize_end(EndInst &end_inst,
                                      llvm::Use &use,
                                      RemoveInst &remove_inst) {
  MemOIRBuilder builder(remove_inst);

  auto *size_inst = builder.CreateSizeInst(&remove_inst.getBaseCollection());
  MEMOIR_NULL_CHECK(size_inst,
                    "Could not contextualize EndInst for RemoveInst!");

  use.set(&size_inst->getCallInst());

  return size_inst->getCallInst();
}

static llvm::Value &contextualize_end(EndInst &end_inst,
                                      llvm::Use &use,
                                      CopyInst &copy_inst) {
  MemOIRBuilder builder(copy_inst);

  auto *size_inst = builder.CreateSizeInst(&copy_inst.getCopiedCollection());
  MEMOIR_NULL_CHECK(size_inst, "Could not contextualize EndInst for CopyInst!");

  use.set(&size_inst->getCallInst());

  return size_inst->getCallInst();
}

static llvm::Value &contextualize_end(EndInst &end_inst,
                                      llvm::Use &use,
                                      SeqSwapInst &swap_inst) {
  MemOIRBuilder builder(swap_inst);

  // TODO: this assumes that TO operands will all be _after_ the TO collection
  // operand. Could make this more extensible, but that would require something
  // akin to TableGen.
  auto &collection =
      (use.getOperandNo() > swap_inst.getToCollectionAsUse().getOperandNo())
          ? swap_inst.getToCollection()
          : swap_inst.getFromCollection();

  auto *size_inst = builder.CreateSizeInst(&collection);
  MEMOIR_NULL_CHECK(size_inst, "Could not contextualize EndInst for CopyInst!");

  use.set(&size_inst->getCallInst());

  return size_inst->getCallInst();
}

static llvm::Value &contextualize_end(EndInst &end_inst,
                                      llvm::Use &use,
                                      SeqSwapWithinInst &swap_within_inst) {
  MemOIRBuilder builder(swap_within_inst);

  auto *size_inst =
      builder.CreateSizeInst(&swap_within_inst.getFromCollection());
  MEMOIR_NULL_CHECK(size_inst,
                    "Could not contextualize EndInst for SeqSwapWithinInst!");

  use.set(&size_inst->getCallInst());

  return size_inst->getCallInst();
}

void SSADestructionVisitor::visitEndInst(EndInst &I) {
  // Contextualize EndInst for each of its users.
  for (auto &use : I.getCallInst().uses()) {
    auto *user = use.getUser();
    auto *user_as_inst = dyn_cast_or_null<llvm::Instruction>(user);
    if (!user_as_inst) {
      // We don't care about non-instruction users.
      continue;
    }

    // Handle end in the context of its use.
    if (auto *insert_inst = into<InsertInst>(user_as_inst)) {
      auto &contextualized = contextualize_end(I, use, *insert_inst);
    } else if (auto *remove_inst = into<RemoveInst>(user_as_inst)) {
      auto &contextualized = contextualize_end(I, use, *remove_inst);
    } else if (auto *copy_inst = into<CopyInst>(user_as_inst)) {
      auto &contextualized = contextualize_end(I, use, *copy_inst);
    } else if (auto *swap_inst = into<SeqSwapInst>(user_as_inst)) {
      auto &contextualized = contextualize_end(I, use, *swap_inst);
    } else if (auto *swap_within_inst = into<SeqSwapWithinInst>(user_as_inst)) {
      auto &contextualized = contextualize_end(I, use, *swap_within_inst);
    } else if (auto *phi_node = dyn_cast<llvm::PHINode>(user_as_inst)) {
      MEMOIR_UNREACHABLE(
          "Contextualizing EndInst at a PHINode is not yet supported!");
    } else if (auto *call = dyn_cast<llvm::CallBase>(user_as_inst)) {
      MEMOIR_UNREACHABLE(
          "Contextualizing EndInst intraprocedurally is not yet supported!");
    } else {
      println(*user_as_inst);
      MEMOIR_UNREACHABLE(
          "Unknown user of EndInst, above use is not yet supported!");
    }
  }

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitIndexReadInst(IndexReadInst &I) {
  if (this->enable_collection_lowering) {
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
  } else {
  }
  return;
}

void SSADestructionVisitor::visitIndexGetInst(IndexGetInst &I) {
  if (this->enable_collection_lowering) {
    // Get a builder.
    MemOIRBuilder builder(I);

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
  } else {
  }
  return;
}

void SSADestructionVisitor::visitIndexWriteInst(IndexWriteInst &I) {
  MemOIRBuilder builder(I);

  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of written collection");

  if (this->enable_collection_lowering) {
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

      auto *write_type = function_type->getParamType(2);
      auto *write_value =
          (isa<llvm::IntegerType>(write_type))
              ? &I.getValueWritten()
              : builder.CreateBitOrPointerCast(&I.getValueWritten(),
                                               write_type);

      auto *llvm_call = builder.CreateCall(
          function_callee,
          llvm::ArrayRef({ vector_value, vector_index, write_value }));
      MEMOIR_NULL_CHECK(llvm_call,
                        "Could not create the call for vector write");

      // Coalesce the input operand with the result of the defPHI.
      this->coalesce(I.getCollection(), I.getObjectOperand());

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

      // Coalesce the input operand with the result of the defPHI.
      this->coalesce(I.getCollection(), I.getObjectOperand());

      // Cleanup the old instruction.
      this->markForCleanup(I);
    }
  } else {
    // Get type information.
    auto &element_type = collection_type.getElementType();

    // Get operands.
    auto &value_written = I.getValueWritten();
    auto &collection = I.getObjectOperand();
    auto &index = I.getIndexOfDimension(0);

    // Construct the MutWriteInst.
    auto *mut_write = builder.CreateMutIndexWriteInst(element_type,
                                                      &value_written,
                                                      &collection,
                                                      &index);

    // Coalesce the original collection with the operand.
    this->coalesce(I, collection);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }
  return;
}

// Assoc accesses lowering implementation.
void SSADestructionVisitor::visitAssocReadInst(AssocReadInst &I) {
  if (this->enable_collection_lowering) {
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
    auto *assoc_value =
        builder.CreatePointerCast(&I.getObjectOperand(),
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
  } else {
  }
  return;
}

void SSADestructionVisitor::visitAssocWriteInst(AssocWriteInst &I) {
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of written collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  MemOIRBuilder builder(I);

  if (this->enable_collection_lowering) {
    auto key_code = key_type.get_code();
    auto value_code = value_type.get_code();
    auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__write";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find AssocWrite name for ", name);
      return;
    }

    auto *function_type = function_callee.getFunctionType();
    auto *assoc_value =
        builder.CreatePointerCast(&I.getObjectOperand(),
                                  function_type->getParamType(0));
    auto *assoc_index =
        builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                       function_type->getParamType(1));
    auto *write_value = &I.getValueWritten();

    auto *llvm_call = builder.CreateCall(
        function_callee,
        llvm::ArrayRef({ assoc_value, assoc_index, write_value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocWrite");

    // Coalesce the input operand with the result of the defPHI.
    this->coalesce(I.getCollection(), I.getObjectOperand());

    this->markForCleanup(I);

  } else {
    // Get operands.
    auto &value_written = I.getValueWritten();
    auto &collection = I.getObjectOperand();
    auto &key = I.getKeyOperand();

    // Construct the MutWriteInst.
    auto *mut_write = builder.CreateMutAssocWriteInst(value_type,
                                                      &value_written,
                                                      &collection,
                                                      &key);

    // Coalesce the original collection with the operand.
    this->coalesce(I, collection);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }

  return;
}

void SSADestructionVisitor::visitAssocGetInst(AssocGetInst &I) {
  if (this->enable_collection_lowering) {
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
    auto *assoc_value =
        builder.CreatePointerCast(&I.getObjectOperand(),
                                  function_type->getParamType(0));
    auto *assoc_key =
        builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                       function_type->getParamType(1));

    auto *llvm_call =
        builder.CreateCall(function_callee,
                           llvm::ArrayRef({ assoc_value, assoc_key }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRead");

    auto *return_type = I.getCallInst().getType();
    if (!return_type->isVoidTy()) {
      auto *collection =
          builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

      this->coalesce(I, *collection);
    }

    this->markForCleanup(I);
  } else {
  }
  return;
}

void SSADestructionVisitor::visitAssocHasInst(AssocHasInst &I) {
  if (this->enable_collection_lowering) {
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
    auto *assoc_value =
        builder.CreatePointerCast(&I.getObjectOperand(),
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
  } else {
  }
  return;
}

// Struct access lowering.
void SSADestructionVisitor::visitStructReadInst(StructReadInst &I) {
  if (this->enable_collection_lowering) {
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
    auto *ptr =
        builder.CreatePointerCast(&struct_value,
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
  } else {
    // Do nothing.
  }

  return;
}

void SSADestructionVisitor::visitStructWriteInst(StructWriteInst &I) {
  if (this->enable_collection_lowering) {
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
    auto *ptr =
        builder.CreatePointerCast(&struct_value,
                                  llvm::PointerType::get(&llvm_type, 0));

    // Construct the GEP for the field.
    auto *gep = builder.CreateStructGEP(ptr, field_offset);

    // Get the value being written.
    auto *value_written = &I.getValueWritten();

    // If the field is a bit field, load the resident value, perform the
    // requisite bit twiddling, and then store the value.
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

    // Cast the value written to match the gep type.
    if (auto *gep_ptr_type = dyn_cast<llvm::PointerType>(gep->getType())) {
      // Get the element type.
      auto *elem_type = gep_ptr_type->getElementType();

      // Create a Bit/PointerCast for non-integer types.
      if (!isa<llvm::IntegerType>(elem_type)) {
        value_written =
            builder.CreateBitOrPointerCast(value_written, elem_type);
      }
    }

    // Construct the load.
    auto &store = MEMOIR_SANITIZE(
        builder.CreateStore(value_written, gep, /* isVolatile = */ false),
        "Failed to create the LLVM store for StructWriteInst");

    // Coalesce and return.
    this->coalesce(I, store);

    // I.getCallInst().replaceAllUsesWith(&store);

    this->markForCleanup(I);
  } else {
    // Do nothing.
  }

  return;
}

void SSADestructionVisitor::visitStructGetInst(StructGetInst &I) {
  if (this->enable_collection_lowering) {
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
    auto *ptr =
        builder.CreatePointerCast(&struct_value,
                                  llvm::PointerType::get(&llvm_type, 0));

    // Construct the GEP for the field.
    auto *gep = builder.CreateStructGEP(ptr, field_offset);

    // If the field is a bit field, load the resident value, perform the
    // requisite bit twiddling, and then store the value.
    if (struct_layout.is_bit_field(field_index)) {
      MEMOIR_UNREACHABLE("Nested objects cannot be bit fields!");
    }

    // Coalesce and return.
    this->coalesce(I, *gep);

    this->markForCleanup(I);
  } else {
    // Do nothing.
  }

  return;
}

// Sequence operations lowering implementation.
void SSADestructionVisitor::visitSeqInsertInst(SeqInsertInst &I) {
  auto &seq_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<SequenceType>(
                          TypeAnalysis::analyze(I.getBaseCollection())),
                      "Couldn't determine type of written collection");

  auto &elem_type = seq_type.getElementType();

  MemOIRBuilder builder(I);

  if (this->enable_collection_lowering) {
    auto elem_code = elem_type.get_code();
    auto name = *elem_code + "_" SEQ_IMPL "__insert_element";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find function for ", name);
      return;
    }

    auto *function_type = function_callee.getFunctionType();
    auto *seq = builder.CreatePointerCast(&I.getBaseCollection(),
                                          function_type->getParamType(0));
    auto *insertion_point =
        builder.CreateBitOrPointerCast(&I.getInsertionPoint(),
                                       function_type->getParamType(1));

    auto *value_param_type = function_type->getParamType(2);
    auto *insertion_value =
        (isa<llvm::IntegerType>(value_param_type))
            ? builder.CreateZExtOrTrunc(&I.getValueInserted(), value_param_type)
            : builder.CreateBitOrPointerCast(&I.getValueInserted(),
                                             value_param_type);

    auto *llvm_call = builder.CreateCall(
        function_callee,
        llvm::ArrayRef({ seq, insertion_point, insertion_value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for SeqInsertInst");

    auto *return_type = I.getResultCollection().getType();
    if (!return_type->isVoidTy()) {
      auto *collection = builder.CreatePointerCast(llvm_call, return_type);

      // Coalesce the result with the input operand.
      this->coalesce(I, *collection);
    }

    // Mark the old instruction for cleanup.
    this->markForCleanup(I);
  } else {
    // Get operands.
    auto &value_inserted = I.getValueInserted();
    auto &collection = I.getBaseCollection();
    auto &index = I.getInsertionPoint();

    // Construct the MutWriteInst.
    auto *mut_inst = builder.CreateMutSeqInsertInst(elem_type,
                                                    &value_inserted,
                                                    &collection,
                                                    &index);

    // Coalesce the original collection with the operand.
    this->coalesce(I, collection);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }
  return;
}

void SSADestructionVisitor::visitSeqInsertSeqInst(SeqInsertSeqInst &I) {
  MemOIRBuilder builder(I);

  if (this->enable_collection_lowering) {
    auto &seq_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<SequenceType>(
                            TypeAnalysis::analyze(I.getBaseCollection())),
                        "Couldn't determine type of written collection");

    auto &elem_type = seq_type.getElementType();

    auto elem_code = elem_type.get_code();
    // TODO: check if we are inserting a copy/view, if we are, remove the copy
    // and use *__insert_range
    auto name = *elem_code + "_" SEQ_IMPL "__insert";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find function for ", name);
      return;
    }

    auto *function_type = function_callee.getFunctionType();
    auto *seq = builder.CreatePointerCast(&I.getBaseCollection(),
                                          function_type->getParamType(0));
    auto *insertion_point =
        builder.CreateBitOrPointerCast(&I.getInsertionPoint(),
                                       function_type->getParamType(1));
    auto *seq_to_insert =
        builder.CreateBitOrPointerCast(&I.getInsertedCollection(),
                                       function_type->getParamType(2));

    auto *llvm_call = builder.CreateCall(
        function_callee,
        llvm::ArrayRef({ seq, insertion_point, seq_to_insert }));
    MEMOIR_NULL_CHECK(llvm_call,
                      "Could not create the call for SeqInsertSeqInst");

    auto *return_type = I.getResultCollection().getType();
    if (!return_type->isVoidTy()) {
      auto *collection = builder.CreatePointerCast(llvm_call, return_type);

      // Coalesce the result with the input operand.
      this->coalesce(I, *collection);
    }
    // Mark the old instruction for cleanup.
    this->markForCleanup(I);
  } else {
    // Get operands.
    auto &inserted_collection = I.getInsertedCollection();
    auto &collection = I.getBaseCollection();
    auto &index = I.getInsertionPoint();

    // Construct the Mut instruction.
    auto *mut_inst = builder.CreateMutSeqInsertSeqInst(&inserted_collection,
                                                       &collection,
                                                       &index);

    // Coalesce the original collection with the operand.
    this->coalesce(I, collection);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }
  return;
}

void SSADestructionVisitor::visitSeqRemoveInst(SeqRemoveInst &I) {
  MemOIRBuilder builder(I);

  if (this->enable_collection_lowering) {
    auto &seq_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<SequenceType>(
                            TypeAnalysis::analyze(I.getBaseCollection())),
                        "Couldn't determine type of written collection");

    auto &elem_type = seq_type.getElementType();

    auto elem_code = elem_type.get_code();
    // TODO: check if we statically know that this is a single element. If it
    // is, we make this a *__remove
    auto name = *elem_code + "_" SEQ_IMPL "__remove_range";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find function for ", name);
      return;
    }

    auto *function_type = function_callee.getFunctionType();
    auto *seq = builder.CreatePointerCast(&I.getBaseCollection(),
                                          function_type->getParamType(0));
    auto *begin =
        builder.CreateBitOrPointerCast(&I.getBeginIndex(),
                                       function_type->getParamType(1));
    auto *end = builder.CreateBitOrPointerCast(&I.getEndIndex(),
                                               function_type->getParamType(2));

    auto *llvm_call = builder.CreateCall(function_callee,
                                         llvm::ArrayRef({ seq, begin, end }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for SeqRemoveInst");

    auto *return_type = I.getResultCollection().getType();
    if (!return_type->isVoidTy()) {
      auto *collection = builder.CreatePointerCast(llvm_call, return_type);

      // Coalesce the result with the input operand.
      this->coalesce(I, *collection);
    }

    // Mark the old instruction for cleanup.
    this->markForCleanup(I);
  } else {
    // Get operands.
    auto &collection = I.getBaseCollection();
    auto &begin_index = I.getBeginIndex();
    auto &end_index = I.getEndIndex();

    // Construct the Mut instruction.
    auto *mut_inst =
        builder.CreateMutSeqRemoveInst(&collection, &begin_index, &end_index);

    // Coalesce the original collection with the operand.
    this->coalesce(I, collection);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }
  return;
}

void SSADestructionVisitor::visitSeqCopyInst(SeqCopyInst &I) {
  MemOIRBuilder builder(I);

  // TODO: reintroduce Views for optimizations.
  if (this->enable_collection_lowering) {
    auto &seq_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<SequenceType>(
                            TypeAnalysis::analyze(I.getCopiedCollection())),
                        "Couldn't determine type of written collection");

    auto &elem_type = seq_type.getElementType();

    auto elem_code = elem_type.get_code();
    auto name = *elem_code + "_" SEQ_IMPL "__copy";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find function for ", name);
      return;
    }

    auto *function_type = function_callee.getFunctionType();
    auto *seq = builder.CreatePointerCast(&I.getCopiedCollection(),
                                          function_type->getParamType(0));
    auto *begin =
        builder.CreateBitOrPointerCast(&I.getBeginIndex(),
                                       function_type->getParamType(1));
    auto *end = builder.CreateBitOrPointerCast(&I.getEndIndex(),
                                               function_type->getParamType(2));
    auto *llvm_call = builder.CreateCall(function_callee,
                                         llvm::ArrayRef({ seq, begin, end }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for SeqCopyInst");

    auto *return_type = I.getCopy().getType();
    if (!return_type->isVoidTy()) {
      auto *collection = builder.CreatePointerCast(llvm_call, return_type);

      // Coalesce the result with the input operand.
      this->coalesce(I, *collection);
    }

    // Mark the old instruction for cleanup.
    this->markForCleanup(I);
  } else {
    // Do nothing.
  }

  return;
}

void SSADestructionVisitor::visitSeqSwapInst(SeqSwapInst &I) {
  MemOIRBuilder builder(I);

  if (this->enable_collection_lowering) {
    auto &seq_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<SequenceType>(
                            TypeAnalysis::analyze(I.getFromCollection())),
                        "Couldn't determine type of written collection");

    auto &elem_type = seq_type.getElementType();

    auto elem_code = elem_type.get_code();
    auto name = *elem_code + "_" SEQ_IMPL "__swap";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find function for ", name);
      return;
    }

    auto *function_type = function_callee.getFunctionType();
    auto *seq = builder.CreatePointerCast(&I.getFromCollection(),
                                          function_type->getParamType(0));
    auto *begin =
        builder.CreateBitOrPointerCast(&I.getBeginIndex(),
                                       function_type->getParamType(1));
    auto *end = builder.CreateBitOrPointerCast(&I.getEndIndex(),
                                               function_type->getParamType(2));
    auto *to_seq = builder.CreatePointerCast(&I.getToCollection(),
                                             function_type->getParamType(3));
    auto *to_begin =
        builder.CreateBitOrPointerCast(&I.getToBeginIndex(),
                                       function_type->getParamType(4));

    auto *llvm_call = builder.CreateCall(
        function_callee,
        llvm::ArrayRef({ seq, begin, end, to_seq, to_begin }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for SeqSwapInst");

    // Coalesce the results with the input operands.
    auto &result_pair = I.getResult();
    for (auto &use : result_pair.uses()) {
      auto *user = use.getUser();
      auto *user_as_inst = dyn_cast_or_null<llvm::Instruction>(user);
      if (!user_as_inst) {
        continue;
      }
      if (auto *extract_value =
              dyn_cast<llvm::ExtractValueInst>(user_as_inst)) {
        auto extract_index = *(extract_value->idx_begin());
        switch (extract_index) {
          case 0:
            this->coalesce(*extract_value, I.getFromCollection());
            this->markForCleanup(*extract_value);
            break;
          case 1:
            this->coalesce(*extract_value, I.getToCollection());
            this->markForCleanup(*extract_value);
            break;
          default:
            break;
        }
      } else if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
        MEMOIR_UNREACHABLE(
            "Result pair from SeqSwapInst used by PHI, tell Tommy to implement this.");
      } else {
        MEMOIR_UNREACHABLE("Result pair used by unknown instruction!");
      }
    }

    // Mark the old instruction for cleanup.
    this->markForCleanup(I);
  } else {
    // Get operands.
    auto &from_collection = I.getFromCollection();
    auto &begin_index = I.getBeginIndex();
    auto &end_index = I.getEndIndex();
    auto &to_collection = I.getToCollection();
    auto &to_begin_index = I.getToBeginIndex();

    // Construct the Mut instruction.
    auto *mut_inst = builder.CreateMutSeqSwapInst(&from_collection,
                                                  &begin_index,
                                                  &end_index,
                                                  &to_collection,
                                                  &to_begin_index);
    // Coalesce the original collections with the operand.
    for (auto &use : I.getCallInst().uses()) {
      auto *user = use.getUser();
      if (auto *extract_value =
              dyn_cast_or_null<llvm::ExtractValueInst>(user)) {
        // Get the index of the value being extracted.
        // NOTE: 0 = FROM, 1 = TO
        auto aggregate_index = *(extract_value->idx_begin());

        // Coalesce the extracted value with the mutable collection.
        this->coalesce(
            *extract_value,
            (aggregate_index == 0) ? from_collection : to_collection);

        // Cleanup the old extracted value.
        this->markForCleanup(*extract_value);
      }
    }

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }
  return;
}

void SSADestructionVisitor::visitSeqSwapWithinInst(SeqSwapWithinInst &I) {
  MemOIRBuilder builder(I);

  if (this->enable_collection_lowering) {
    auto &seq_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<SequenceType>(
                            TypeAnalysis::analyze(I.getFromCollection())),
                        "Couldn't determine type of written collection");

    auto &elem_type = seq_type.getElementType();

    auto elem_code = elem_type.get_code();
    auto name = *elem_code + "_" SEQ_IMPL "__swap";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find function for ", name);
      return;
    }

    auto *function_type = function_callee.getFunctionType();
    auto *seq = builder.CreatePointerCast(&I.getFromCollection(),
                                          function_type->getParamType(0));
    auto *begin =
        builder.CreateBitOrPointerCast(&I.getBeginIndex(),
                                       function_type->getParamType(1));
    auto *end = builder.CreateBitOrPointerCast(&I.getEndIndex(),
                                               function_type->getParamType(2));
    auto *to_begin =
        builder.CreateBitOrPointerCast(&I.getToBeginIndex(),
                                       function_type->getParamType(4));
    auto *llvm_call =
        builder.CreateCall(function_callee,
                           llvm::ArrayRef({ seq, begin, end, seq, to_begin }));
    MEMOIR_NULL_CHECK(llvm_call,
                      "Could not create the call for SeqSwapWithinInst");

    auto *return_type = llvm_call->getType();
    if (!return_type->isVoidTy()) {
      auto *collection =
          builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

      // Coalesce the result with the original resultant.
      this->coalesce(I.getResult(), *collection);
    } else {
      // Coalesce the result with the input operand.
      this->coalesce(I.getResult(), I.getFromCollection());
    }

    // Mark the old instruction for cleanup.
    this->markForCleanup(I);
  } else {
    // Get operands.
    auto &collection = I.getFromCollection();
    auto &begin_index = I.getBeginIndex();
    auto &end_index = I.getEndIndex();
    auto &to_begin_index = I.getToBeginIndex();

    // Construct the Mut instruction.
    auto *mut_inst = builder.CreateMutSeqSwapWithinInst(&collection,
                                                        &begin_index,
                                                        &end_index,
                                                        &to_begin_index);

    // Coalesce the original collection with the operand.
    this->coalesce(I, collection);

    // Cleanup the old instruction.
    this->markForCleanup(I);
  }

  return;
}

// Assoc operations lowering implementation.
void SSADestructionVisitor::visitAssocInsertInst(AssocInsertInst &I) {
  if (this->enable_collection_lowering) {
    auto &assoc_type = MEMOIR_SANITIZE(
        dyn_cast_or_null<AssocArrayType>(
            TypeAnalysis::analyze(I.getBaseCollection())),
        "Couldn't determine type of collection being inserted into.");

    auto &key_type = assoc_type.getKeyType();
    auto &value_type = assoc_type.getValueType();

    auto key_code = key_type.get_code();
    auto value_code = value_type.get_code();
    auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__insert";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find function for ", name);
      return;
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *assoc = builder.CreatePointerCast(&I.getBaseCollection(),
                                            function_type->getParamType(0));
    auto *assoc_key =
        builder.CreateBitOrPointerCast(&I.getInsertionPoint(),
                                       function_type->getParamType(1));

    auto *llvm_call = builder.CreateCall(function_callee,
                                         llvm::ArrayRef({ assoc, assoc_key }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRemove");

    auto *return_type = I.getCallInst().getType();
    if (!return_type->isVoidTy()) {
      auto *collection =
          builder.CreatePointerCast(llvm_call,
                                    I.getResultCollection().getType());

      // Coalesce the result with the input operand.
      this->coalesce(I.getResultCollection(), I.getBaseCollection());
    }

    this->markForCleanup(I);

  } else {
    // Do nothing.
  }
  return;
}

void SSADestructionVisitor::visitAssocRemoveInst(AssocRemoveInst &I) {
  if (this->enable_collection_lowering) {
    auto &assoc_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                            TypeAnalysis::analyze(I.getBaseCollection())),
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
    auto *assoc = builder.CreatePointerCast(&I.getBaseCollection(),
                                            function_type->getParamType(0));
    auto *assoc_key =
        builder.CreateBitOrPointerCast(&I.getKey(),
                                       function_type->getParamType(1));

    auto *llvm_call = builder.CreateCall(function_callee,
                                         llvm::ArrayRef({ assoc, assoc_key }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRemove");

    auto *return_type = I.getCallInst().getType();
    if (!return_type->isVoidTy()) {
      // TODO: this may need more work.
      auto *collection =
          builder.CreatePointerCast(llvm_call,
                                    I.getResultCollection().getType());

      // Coalesce the result with the input operand.
      this->coalesce(I.getResultCollection(), I.getBaseCollection());
    }

    this->markForCleanup(I);
  } else {
    // Construct the MutAssocRemoveInst.
  }
  return;
}

void SSADestructionVisitor::visitAssocKeysInst(AssocKeysInst &I) {
  if (this->enable_collection_lowering) {
    auto &assoc_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                            TypeAnalysis::analyze(I.getCollection())),
                        "Couldn't determine type assoc collection");

    auto &key_type = assoc_type.getKeyType();
    auto &value_type = assoc_type.getValueType();

    auto key_code = key_type.get_code();
    auto value_code = value_type.get_code();
    auto name = *key_code + "_" + *value_code + "_" ASSOC_IMPL "__keys";

    auto *function = this->M.getFunction(name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find AssocKeys name for ", name);
      return;
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *assoc = builder.CreatePointerCast(&I.getCollection(),
                                            function_type->getParamType(0));

    auto *llvm_call =
        builder.CreateCall(function_callee, llvm::ArrayRef({ assoc }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocKeys");

    auto *collection =
        builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

    this->coalesce(I, *collection);

    this->markForCleanup(I);
  } else {
    // Do nothing.
  }
  return;
}

// General-purpose SSA lowering.
void SSADestructionVisitor::visitUsePHIInst(UsePHIInst &I) {

  auto &used_collection = I.getUsedCollection();
  auto &collection = I.getResultCollection();

  this->coalesce(collection, used_collection);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitDefPHIInst(DefPHIInst &I) {

  auto &defined_collection = I.getDefinedCollection();
  auto &collection = I.getResultCollection();

  auto found_replacement = this->def_phi_replacements.find(&I.getCallInst());
  if (found_replacement != this->def_phi_replacements.end()) {
    this->coalesce(collection, *found_replacement->second);
  } else {
    this->coalesce(collection, defined_collection);
  }

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitArgPHIInst(ArgPHIInst &I) {

  auto &input_collection = I.getInputCollection();
  auto &collection = I.getResultCollection();

  this->coalesce(collection, input_collection);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitRetPHIInst(RetPHIInst &I) {

  auto &input_collection = I.getInputCollection();
  auto &collection = I.getResultCollection();

  auto found_replacement = this->ret_phi_replacements.find(&collection);
  if (found_replacement != this->ret_phi_replacements.end()) {
    this->coalesce(collection, *found_replacement->second);
  } else {
    this->coalesce(collection, input_collection);
  }

  this->markForCleanup(I);

  return;
}

// Type erasure.
void SSADestructionVisitor::visitAssertCollectionTypeInst(
    AssertCollectionTypeInst &I) {
  if (this->enable_collection_lowering) {
    this->markForCleanup(I);
  } else {
    // Do nothing.
  }
  return;
}

void SSADestructionVisitor::visitAssertStructTypeInst(AssertStructTypeInst &I) {
  if (this->enable_collection_lowering) {
    this->markForCleanup(I);
  } else {
    // Do nothing.
  }
  return;
}

void SSADestructionVisitor::visitReturnTypeInst(ReturnTypeInst &I) {
  if (this->enable_collection_lowering) {
    this->markForCleanup(I);
  } else {
    // Do nothing.
  }
  return;
}

void SSADestructionVisitor::visitTypeInst(TypeInst &I) {
  if (this->enable_collection_lowering) {
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
          auto *ptr_user_as_inst =
              dyn_cast_or_null<llvm::Instruction>(ptr_user);
          if (!ptr_user_as_inst) {
            continue;
          }

          // Mark the user for cleanup.
          this->markForCleanup(*ptr_user_as_inst);
        }
      }
    }

    this->markForCleanup(I);
  } else {
  }
  return;
}

// Logistics implementation.
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
