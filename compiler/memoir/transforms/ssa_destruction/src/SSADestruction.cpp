#include <string>

#include "llvm/IR/DerivedTypes.h"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/lowering/ImplLinker.hpp"
#include "memoir/lowering/Implementation.hpp"
#include "memoir/lowering/TypeLayout.hpp"

#include "memoir/lowering/LowerFold.hpp"

#include "SSADestruction.hpp"

namespace llvm::memoir {

namespace detail {

/**
 * Fetch the Implementation from either the selection metadata or the type if no
 * metadata exists.
 */
static const Implementation &get_implementation(
    const std::optional<std::string> &selection,
    CollectionType &type) {

  // Lookup the selection, if we were given one.
  if (selection.has_value()) {
    return MEMOIR_SANITIZE(
        Implementation::lookup(selection.value()),
        "Requested implementation has not been registered with the compiler!");
  }

  // Otherwise, get the default implementation.
  return ImplLinker::get_default_implementation(type);
}

static std::string get_implementation_prefix(const Implementation &impl,
                                             CollectionType &type) {
  if (auto *assoc_type = dyn_cast<AssocType>(&type)) {
    auto &key_type = assoc_type->getKeyType();
    auto key_code = key_type.get_code();

    auto &element_type = assoc_type->getElementType();
    auto element_code = element_type.get_code();

    return *key_code + "_" + *element_code + "_" + impl.get_name();

  } else if (auto *seq_type = dyn_cast<SequenceType>(&type)) {
    auto &element_type = seq_type->getElementType();
    auto element_code = element_type.get_code();

    return *element_code + "_" + impl.get_name();
  } else {
    MEMOIR_UNREACHABLE("Unhandled collection type.");
  }
}

FunctionCallee get_function_callee(llvm::Module &M, std::string name) {
  auto *function = M.getFunction(name);
  if (not function) {
    println("Couldn't find ", name);
    MEMOIR_UNREACHABLE("see above");
  }

  return FunctionCallee(function);
}

llvm::Value &construct_field_read(MemOIRBuilder &builder,
                                  llvm::Type &result_type,
                                  StructType &type,
                                  TypeLayout &layout,
                                  llvm::Value &object,
                                  unsigned field_index) {

  // Fetch the LLVM struct type.
  auto *llvm_type = cast<llvm::StructType>(&layout.get_llvm_type());

  // Fetch the LLVM field type.
  auto field_offset = layout.get_field_offset(field_index);
  auto *llvm_field_type = llvm_type->getElementType(field_offset);

  // Construct a pointer cast to the LLVM struct type.
  // auto *ptr =
  // builder.CreatePointerCast(&object, llvm::PointerType::get(llvm_type, 0));
  auto *ptr = &object;

  // Construct the GEP for the field.
  auto *gep = builder.CreateStructGEP(llvm_type, ptr, field_offset);

  // Construct the load.
  llvm::Value *load =
      builder.CreateLoad(llvm_field_type, gep, /* isVolatile = */ false);

  // If the field is a bit field, pay the bit twiddler their due.
  if (layout.is_bit_field(field_index)) {
    // Fetch the bit field range.
    auto bit_field_range = *(layout.get_bit_field_range(field_index));
    auto bit_field_start = bit_field_range.first;
    auto bit_field_end = bit_field_range.second;
    auto bit_field_width = bit_field_end - bit_field_start;

    // If the field is signed, we need to become the king bit twiddler.
    bool is_signed = false;
    auto &field_type = type.getFieldType(field_index);

    if (auto *int_field_type = dyn_cast<IntegerType>(&field_type)) {
      if (int_field_type->isSigned()) {
        is_signed = true;

        // Get the size of the containing bit field.
        auto *llvm_int_field_type = cast<llvm::IntegerType>(&result_type);
        auto llvm_field_width = llvm_int_field_type->getBitWidth();

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
    for (unsigned int i = 0; i < bit_field_width; ++i) {
      mask |= 1 << i;
    }
    load = builder.CreateAnd(load, mask);

    // BITCAST the value, if needed.
    load = builder.CreateIntCast(load, &result_type, is_signed);
  }

  return *load;
}

void construct_field_write(MemOIRBuilder &builder,
                           StructType &type,
                           TypeLayout &layout,
                           llvm::Value &object,
                           unsigned field_index,
                           llvm::Value &value_to_write) {

  // Unpack the type layout.
  auto field_offset = layout.get_field_offset(field_index);

  auto &llvm_type = cast<llvm::StructType>(layout.get_llvm_type());
  auto *llvm_field_type = llvm_type.getElementType(field_offset);

  // Construct a pointer cast to the LLVM struct type.
  // auto *ptr =
  // builder.CreatePointerCast(&object, llvm::PointerType::get(&llvm_type, 0));
  auto *ptr = &object;

  // Construct the GEP for the field.
  auto *gep = builder.CreateStructGEP(&llvm_type, ptr, field_offset);

  auto *value_written = &value_to_write;

  // If the field is a bit field, load the resident value, perform the
  // requisite bit twiddling, and then store the value.
  if (layout.is_bit_field(field_index)) {
    llvm::Value *load = builder.CreateLoad(llvm_field_type, gep);

    // Fetch the bit field range.
    auto bit_field_range = *(layout.get_bit_field_range(field_index));
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

  // Cast the value written to match the gep type if it's a non-integer type.
  if (not isa<llvm::IntegerType>(llvm_field_type)) {
    value_written =
        builder.CreateTruncOrBitCast(value_written, llvm_field_type);
  }

  // Construct the load.
  auto &store = MEMOIR_SANITIZE(
      builder.CreateStore(value_written, gep, /* isVolatile = */ false),
      "Failed to create the LLVM store for field write");
}

/**
 * @param function_name the function to find and prepare
 * @param arguments, the list of arguments to prepare for the call
 * @returns the function callee for the given function name.
 */
static FunctionCallee prepare_call(MemOIRBuilder &builder,
                                   const Implementation &implementation,
                                   CollectionType &type,
                                   const std::string &operation,
                                   vector<llvm::Value *> &arguments) {

  auto prefix = detail::get_implementation_prefix(implementation, type);
  auto function_name = prefix + "__" + operation;

  auto callee = detail::get_function_callee(builder.getModule(), function_name);
  auto *function_type = callee.getFunctionType();

  auto arg_index = 0;
  for (auto &arg : arguments) {
    // Prepare the argument.
    auto *param_type = function_type->getParamType(arg_index);

    // If the argument doesn't match the parameter's type, prepare it.
    if (arg->getType() != param_type) {
      if (isa<llvm::IntegerType>(param_type)) {
        arg = builder.CreateZExtOrTrunc(arg, param_type);
      } else {
        MEMOIR_UNREACHABLE("Unhandled type mismatch!");
      }
    }

    ++arg_index;
  }

  return callee;
}

} // namespace detail

SSADestructionVisitor::SSADestructionVisitor(llvm::Module &M,
                                             SSADestructionStats *stats)
  : M(M),
    TC(M.getContext()),
    stats(stats) {
  // Do nothing.
}

void SSADestructionVisitor::setAnalyses(llvm::DominatorTree &DT) {
  this->DT = &DT;

  return;
}

void SSADestructionVisitor::visitInstruction(llvm::Instruction &I) {
  return;
}

void SSADestructionVisitor::visitAllocInst(AllocInst &I) {

  MemOIRBuilder builder(I);

  auto *type = &I.getType();

  if (auto *collection_type = dyn_cast<CollectionType>(type)) {

    // Fetch the selection from the instruction metadata, if it exists.
    unsigned selection_index = 0;
    auto selection_metadata = Metadata::get<SelectionMetadata>(I);

    // Track where we are in the size list.
    auto size_it = I.sizes_begin(), size_ie = I.sizes_end();

    // Allocate the nested collections.
    llvm::Value *result = nullptr;
    while (collection_type) {

      // Fetch the collection implementation.
      std::optional<std::string> dim_name = std::nullopt;
      if (selection_metadata.has_value()) {
        selection_metadata->getImplementation(selection_index++);
      }

      const auto &dim_impl =
          detail::get_implementation(dim_name, *collection_type);
      println(dim_impl.get_name());

      // Fetch the arguments.
      vector<llvm::Value *> arguments = {};
      auto *nested_type = collection_type;
      for (unsigned dim = 0; dim < dim_impl.num_dimensions();
           ++dim, ++size_it) {
        auto &elem_type = nested_type->getElementType();
        nested_type = dyn_cast_or_null<CollectionType>(&elem_type);

        if (size_it != size_ie) {
          arguments.push_back(*size_it);
        }
      }

      // Construct the call.
      auto callee = detail::prepare_call(builder,
                                         dim_impl,
                                         *collection_type,
                                         "allocate",
                                         arguments);
      auto &call =
          MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                          "Could not create the call for vector read");

      collection_type = nested_type;

      if (not result) {
        result = &call;
      } else {
        MEMOIR_UNREACHABLE(
            "Initializing nested collections not yet supported!");
      }
    }

    this->coalesce(I, *result);

  } else if (auto *struct_type = dyn_cast<StructType>(type)) {

    // Get the LLVM StructType for this struct.
    auto &type_layout = TC.convert(*struct_type);
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
    // TODO: Get this information from attached metadata, we can safely
    // default to malloc though. auto allocator_name = "malloc";

    // Get the allocator function.
    // auto *allocator_function = this->M.getFunction(allocator_name);
    // MEMOIR_NULL_CHECK(allocator_function, "Couldn't get the allocator
    // function!");

    // Create the allocation.
    auto &call = MEMOIR_SANITIZE(builder.CreateMalloc(int_ptr_type,
                                                      llvm_struct_type,
                                                      llvm_struct_size_constant,
                                                      /* ArraySize = */ nullptr,
                                                      /* MallocF = */ nullptr,
                                                      /* Name = */ "struct."),
                                 "Couldn't create malloc for StructAllocInst");

    this->coalesce(I, call);

  } else {
    MEMOIR_UNREACHABLE("Unhandled type allocation.");
  }

  this->markForCleanup(I);
}

void SSADestructionVisitor::visitDeleteInst(DeleteInst &I) {

  if (auto *collection_type =
          dyn_cast_or_null<CollectionType>(type_of(I.getObject()))) {

    MemOIRBuilder builder(I);

    vector<llvm::Value *> arguments = { &I.getObject() };

    // Fetch the collection implementation.
    std::optional<std::string> impl_name = std::nullopt;
    auto selection_metadata = I.get_keyword<SelectionMetadata>();
    if (selection_metadata.has_value()) {
      impl_name = selection_metadata->getImplementation();
    }

    const auto &impl = detail::get_implementation(impl_name, *collection_type);

    auto callee = detail::prepare_call(builder,
                                       impl,
                                       *collection_type,
                                       "free",
                                       arguments);

    auto *llvm_call = builder.CreateCall(callee, llvm::ArrayRef(arguments));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector read");

    this->markForCleanup(I);
  }

  // TODO: handle freeing nested collections.

  return;
}

// Collect the list of indices before the given use.
static llvm::Value &contextualize_end(AccessInst &inst,
                                      llvm::Use &use,
                                      bool minus_one = false) {
  vector<llvm::Value *> indices = {};
  for (auto &index_use : inst.index_operands()) {
    if (&use != &index_use) {
      indices.push_back(index_use.get());
    }
  }

  MemOIRBuilder builder(inst);

  auto *size_inst = builder.CreateSizeInst(&inst.getObject(), indices);

  // Propagate the selection metadata.
  if (auto selection = Metadata::get<SelectionMetadata>(inst)) {
    auto &metadata = selection->getMetadata();
    auto *clone = llvm::MDNode::replaceWithDistinct(metadata.clone());
    size_inst->getCallInst().setMetadata(
        Metadata::get_kind<SelectionMetadata>(),
        clone);
  }

  llvm::Value *size = &size_inst->getSize();
  if (minus_one) {
    auto *constant_one = llvm::ConstantInt::get(size->getType(), 1);
    size = builder.CreateSub(size, constant_one);
  }

  use.set(size);

  return *size;
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
    if (auto *access = into<AccessInst>(user_as_inst)) {
      // If the operation references a single element, subtract one from the
      // size so we are not off by one.
      bool minus_one = (isa<ReadInst>(access) or isa<WriteInst>(access)
                        or isa<GetInst>(access) or isa<SizeInst>(access));
      contextualize_end(*access, use, minus_one);
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

namespace detail {

struct NestedObjectInfo {
  NestedObjectInfo(llvm::Value &object,
                   Type &type,
                   AccessInst::index_iterator begin,
                   AccessInst::index_iterator end,
                   const Implementation &impl)
    : object(object),
      type(type),
      begin(begin),
      end(end),
      implementation(&impl) {}

  NestedObjectInfo(llvm::Value &object,
                   Type &type,
                   AccessInst::index_iterator begin,
                   AccessInst::index_iterator end)
    : object(object),
      type(type),
      begin(begin),
      end(end),
      implementation(nullptr) {}

  llvm::Value &object;
  Type &type;
  AccessInst::index_iterator begin, end;
  const Implementation *implementation;
};
static NestedObjectInfo get_nested_object(AccessInst &I,
                                          TypeConverter &TC,
                                          llvm::Module &M) {

  MemOIRBuilder builder(I);

  // Unpack the instruction.
  auto *object = &I.getObject();
  auto *type = &I.getObjectType();

  // Fetch the selection from the instruction metadata, if it exists.
  unsigned selection_index = 0;
  auto selection_metadata = Metadata::get<SelectionMetadata>(I);

  // Construct nested access.
  for (auto it = I.indices_begin(), ie = I.indices_end(); it != ie; ++it) {
    auto *index = *it;

    if (auto *struct_type = dyn_cast<StructType>(type)) {

      // Determine the field being accessed.
      auto &index_constant =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                          "Field index is not a constant integer.");
      auto field_index = index_constant.getZExtValue();
      auto &field_type = struct_type->getFieldType(field_index);

      // If the field type is a primitive, we have reached the innermost
      // object.
      if (Type::is_primitive_type(field_type)
          or isa<ReferenceType>(&field_type)) {
        return NestedObjectInfo(*object, *type, it, ie);
      }

      // Fetch the field's selection metadata.
      selection_metadata =
          Metadata::get<SelectionMetadata>(*struct_type, field_index);
      selection_index = 0;

      // Otherwise, construct a get.
      auto &layout = TC.convert(*struct_type);

      // Fetch the LLVM struct type.
      auto *llvm_type = cast<llvm::StructType>(&layout.get_llvm_type());

      // Fetch the LLVM field type.
      auto field_offset = layout.get_field_offset(field_index);
      auto *llvm_field_type = llvm_type->getElementType(field_offset);

      // Construct the GEP for the field.
      object = builder.CreateStructGEP(llvm_type, object, field_offset);
      MEMOIR_NULL_CHECK(object, "Failed to construct GEP instruction");
      type = &field_type;

    } else if (auto *array_type = dyn_cast<ArrayType>(type)) {

      // Unpack the type.
      auto &element_type = array_type->getElementType();
      auto length = array_type->getLength();

      // If the element is a primitive, we have reached the innermost object.
      if (Type::is_primitive_type(element_type)) {
        return NestedObjectInfo(*object, *type, it, ie);
      }

      // Otherwise, construct a get for the inner element.
      auto &layout = TC.convert(*array_type);

      // Fetch the LLVM array type.
      auto *llvm_type = cast<llvm::ArrayType>(&layout.get_llvm_type());

      // Fetch the LLVM element type.
      auto *llvm_element_type = llvm_type->getElementType();

      // Construct the GEP for the element.
      auto *index_type =
          builder.getIndexTy(M.getDataLayout(), /* AddressSpace = */ 0);
      auto *zero_index = builder.getIntN(index_type->getBitWidth(), 0);
      auto *prepared_index = builder.CreateZExtOrTrunc(index, index_type);

      object = builder.CreateInBoundsGEP(
          llvm_element_type,
          object,
          llvm::ArrayRef<llvm::Value *>({ zero_index, prepared_index }));

      // Update the running object type.
      type = &element_type;

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      // Fetch the collection implementation.
      std::optional<std::string> dim_name = std::nullopt;
      if (selection_metadata.has_value()) {
        dim_name = selection_metadata->getImplementation(selection_index++);
      }
      const auto &dim_impl =
          detail::get_implementation(dim_name, *collection_type);

      // Check if the implementation covers the remaining indices or not.
      if (dim_impl.num_dimensions() == std::distance(it, ie)) {
        return NestedObjectInfo(*object, *type, it, ie, dim_impl);
      }

      // Otherwise, we will construct the get operation for the nested object.
      vector<llvm::Value *> arguments = { object, index };
      // TODO: this needs to gather all indices required by the Implementation
      auto &element_type = collection_type->getElementType();

      auto callee = detail::prepare_call(builder,
                                         dim_impl,
                                         *collection_type,
                                         "get",
                                         arguments);

      auto &call =
          MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                          "Could not create the call for get");

      object = &call;
      type = &element_type;

    } else {
      MEMOIR_UNREACHABLE("Dimension mismatch at ",
                         value_name(*index),
                         " in ",
                         I);
    }
  }

  if (isa<StructType>(type) or isa<ArrayType>(type)) {
    return NestedObjectInfo(*object, *type, I.indices_end(), I.indices_end());

  } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
    std::optional<std::string> impl_name = std::nullopt;
    if (selection_metadata.has_value()) {
      impl_name = selection_metadata->getImplementation(selection_index++);
    }
    const auto &impl = detail::get_implementation(impl_name, *collection_type);

    return NestedObjectInfo(*object,
                            *type,
                            I.indices_end(),
                            I.indices_end(),
                            impl);
  }

  MEMOIR_UNREACHABLE("Could not get the nested object for ", I);
}

} // namespace detail

void SSADestructionVisitor::visitReadInst(ReadInst &I) {

  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  llvm::Value *result = nullptr;
  if (auto *struct_type = dyn_cast<StructType>(&info.type)) {
    // There will only ever be a single index for an innermost struct access.
    auto &field_value = MEMOIR_SANITIZE(*info.begin, "Field index is NULL!");
    auto &field_const =
        MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(&field_value),
                        "Field index is not statically known!");
    auto field_index = field_const.getZExtValue();

    // Fetch the type layout for the struct.
    auto &layout = TC.convert(*struct_type);

    // Construct the read.
    result = &detail::construct_field_read(builder,
                                           *I.getCallInst().getType(),
                                           *struct_type,
                                           layout,
                                           info.object,
                                           field_index);

  } else if (auto *array_type = dyn_cast<ArrayType>(&info.type)) {
    // Get the type layout for the tensor type.
    auto &type_layout = TC.convert(*array_type);
    auto &llvm_type = type_layout.get_llvm_type();

    // Construct a gep for the element.
    auto &index = MEMOIR_SANITIZE(*info.begin, "Field index is NULL!");

    auto *index_type =
        builder.getIndexTy(this->M.getDataLayout(), /* AddressSpace = */ 0);
    auto *zero_index = builder.getIntN(index_type->getBitWidth(), 0);
    auto *prepared_index = builder.CreateZExtOrTrunc(&index, index_type);

    auto *gep = builder.CreateInBoundsGEP(
        &llvm_type,
        &info.object,
        llvm::ArrayRef<llvm::Value *>({ zero_index, prepared_index }));

    // Construct the load of the element.
    result = builder.CreateLoad(&llvm_type, gep);
    MEMOIR_NULL_CHECK(result, "Could not create load");

  } else if (auto *collection_type = dyn_cast<CollectionType>(&info.type)) {
    // Fetch the function that implements this operation.
    vector<llvm::Value *> arguments = { &info.object };
    arguments.insert(arguments.end(), info.begin, info.end);

    auto callee = detail::prepare_call(builder,
                                       *info.implementation,
                                       *collection_type,
                                       "read",
                                       arguments);

    result = builder.CreateCall(callee, llvm::ArrayRef(arguments));
    MEMOIR_NULL_CHECK(result, "Could not create the call");

  } else {
    MEMOIR_UNREACHABLE("Unhandled type for nested object.");
  }

  // Coalesce the original with the resultant.
  this->coalesce(I, *result);

  // The instruction is dead now.
  this->markForCleanup(I);
}

void SSADestructionVisitor::visitWriteInst(WriteInst &I) {

  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  if (auto *struct_type = dyn_cast<StructType>(&info.type)) {
    // There will only ever be a single index for an innermost struct access.
    auto &field_value = MEMOIR_SANITIZE(*info.begin, "Field index is NULL!");
    auto &field_const =
        MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(&field_value),
                        "Field index is not statically known!");
    auto field_index = field_const.getZExtValue();

    // Fetch the type layout for the struct.
    auto &layout = TC.convert(*struct_type);

    // Construct the read.
    detail::construct_field_write(builder,
                                  *struct_type,
                                  layout,
                                  info.object,
                                  field_index,
                                  I.getValueWritten());

  } else if (auto *array_type = dyn_cast<ArrayType>(&info.type)) {
    // Get the type layout for the tensor type.
    auto &type_layout = TC.convert(*array_type);
    auto &llvm_type = type_layout.get_llvm_type();

    // Construct a gep for the element.
    auto &index = MEMOIR_SANITIZE(*info.begin, "Field index is NULL!");

    auto *index_type =
        builder.getIndexTy(this->M.getDataLayout(), /* AddressSpace = */ 0);
    auto *zero_index = builder.getIntN(index_type->getBitWidth(), 0);
    auto *prepared_index = builder.CreateZExtOrTrunc(&index, index_type);

    auto *gep = builder.CreateInBoundsGEP(
        &llvm_type,
        &info.object,
        llvm::ArrayRef<llvm::Value *>({ zero_index, prepared_index }));

    // Construct the load of the element.
    builder.CreateStore(&I.getValueWritten(), gep);

  } else if (auto *collection_type = dyn_cast<CollectionType>(&info.type)) {
    // Construct the call.
    vector<llvm::Value *> arguments = { &info.object };
    arguments.insert(arguments.end(), info.begin, info.end);
    arguments.push_back(&I.getValueWritten());

    auto callee = detail::prepare_call(builder,
                                       *info.implementation,
                                       *collection_type,
                                       "write",
                                       arguments);

    builder.CreateCall(callee, llvm::ArrayRef(arguments));

  } else {
    MEMOIR_UNREACHABLE("Unhandled type for nested object.");
  }

  // Coalesce the original with the resultant.
  this->coalesce(I, I.getObject());

  // The instruction is dead now.
  this->markForCleanup(I);
}

void SSADestructionVisitor::visitHasInst(HasInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Construct the call.
  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     "has",
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitInsertInst(InsertInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Fetch the function that implements this operation.
  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  std::string operation_name = "insert";
  if (auto value_kw = I.get_keyword<ValueKeyword>()) {
    operation_name += "_value";
    arguments.push_back(&value_kw->getValue());

  } else if (auto input_kw = I.get_keyword<InputKeyword>()) {
    operation_name += "_input";
    arguments.push_back(&input_kw->getInput());

    if (auto range_kw = I.get_keyword<RangeKeyword>()) {
      operation_name += "_range";
      arguments.push_back(&range_kw->getBegin());
      arguments.push_back(&range_kw->getEnd());
    }
  }

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     operation_name,
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitRemoveInst(RemoveInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Fetch the function that implements this operation.
  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  std::string operation_name = "remove";
  if (auto range_kw = I.get_keyword<RangeKeyword>()) {
    operation_name += "_range";
    arguments.push_back(&range_kw->getBegin());
    arguments.push_back(&range_kw->getEnd());
  }

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     operation_name,
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitCopyInst(CopyInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Fetch the function that implements this operation.
  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  std::string operation_name = "copy";
  if (auto range_kw = I.get_keyword<RangeKeyword>()) {
    operation_name += "_range";
    arguments.push_back(&range_kw->getBegin());
    arguments.push_back(&range_kw->getEnd());
  }

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     operation_name,
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitClearInst(ClearInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Fetch the function that implements this operation.
  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     "clear",
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitSizeInst(SizeInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Fetch the function that implements this operation.
  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     "size",
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitKeysInst(KeysInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Fetch the function that implements this operation.
  auto prefix =
      detail::get_implementation_prefix(*info.implementation, collection_type);

  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     "keys",
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

// Fold instruction.
#if 0
void SSADestructionVisitor::visitFoldInst(FoldInst &I) {

  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, this->M);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Fetch the function that implements this operation.
  auto prefix =
      detail::get_implementation_prefix(*info.implementation, collection_type);

  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  auto callee = detail::prepare_call(builder,
                                     *info.implementation,
                                     collection_type,
                                     "keys",
                                     arguments);

  auto &result =
      MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                      "Could not create the call for ",
                      I);

  // Coalesce the original with the resultant.
  this->coalesce(I, result);

  // The instruction is dead now.
  this->markForCleanup(I);

  // OLD BELOW

  // Fetch the iterator functions.
  auto &collection_type = I.getObjectType();

  auto begin_name = prefix + (I.isReverse() ? "__rbegin" : "__begin");
  auto next_name = prefix + (I.isReverse() ? "__rnext" : "__next");

  // Get the functions to call.
  auto begin_function_callee = detail::get_function_callee(this->M, begin_name);
  auto next_function_callee = detail::get_function_callee(this->M, next_name);

  // Unpack the functions.
  auto *begin_func = cast<llvm::Function>(begin_function_callee.getCallee());
  auto *next_func = cast<llvm::Function>(next_function_callee.getCallee());

  // Fetch the iterator and element types.
  auto iter_struct_name = "struct." + prefix + "_iter";
  auto *iter_type =
      llvm::StructType::getTypeByName(M.getContext(), iter_struct_name);
  if (not iter_type) {
    // If we could not find the iterator type by name, get the next function
    // and find any typed GEPs.
    auto *iter_arg = begin_func->getArg(0);
    for (auto &use : iter_arg->uses()) {
      auto *user = use.getUser();
      if (auto *gep = dyn_cast<llvm::GetElementPtrInst>(user)) {
        if (gep->getPointerOperand() == iter_arg) {
          if (auto *src_type =
                  dyn_cast<llvm::StructType>(gep->getSourceElementType())) {
            if (iter_type == nullptr) {
              iter_type = src_type;
            } else if (iter_type != src_type) {
              iter_type = nullptr;
              break;
            }
          }
        }
      }
    }

    // Ensure that we found a type.
    MEMOIR_NULL_CHECK(iter_type, "Could not infer a type for the iterator!");
  }

  // Invoke the LowerFold utility.
  lower_fold(
      I,
      begin_func,
      next_func,
      iter_type,
      [&](llvm::Value &orig, llvm::Value &replacement) {
        this->coalesce(orig, replacement);
      },
      [&](llvm::Instruction &I) { this->markForCleanup(I); });

  // If the result of the fold is a collection, we need to patch it with the
  // original operand.

  this->markForCleanup(I);

  return;
}
#endif

// General-purpose SSA lowering.
void SSADestructionVisitor::visitUsePHIInst(UsePHIInst &I) {

  auto &used_collection = I.getUsedCollection();
  auto &collection = I.getResultCollection();

  this->coalesce(collection, used_collection);

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

  // If the called instruction is a fold, skip the ret phi!
  auto *function = I.getCalledFunction();
  if (function and FunctionNames::is_memoir_call(*function)) {
    return;
  }

  auto &input_collection = I.getInputCollection();
  auto &collection = I.getResultCollection();

  this->coalesce(collection, input_collection);

  this->markForCleanup(I);

  return;
}

// Type erasure.
void SSADestructionVisitor::visitAssertTypeInst(AssertTypeInst &I) {
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitReturnTypeInst(ReturnTypeInst &I) {
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitTypeInst(TypeInst &I) {

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
      // Mark the store for cleanup.
      this->markForCleanup(*user_as_inst);

      // Get the global variable referenced being stored to.
      llvm::Value *global_ptr = nullptr;
      auto *ptr = user_as_store->getPointerOperand();
      if (auto *ptr_as_global = dyn_cast<llvm::GlobalVariable>(ptr)) {
        global_ptr = ptr_as_global;
      } else if (auto *ptr_as_gep = dyn_cast<llvm::GetElementPtrInst>(ptr)) {
        global_ptr = ptr_as_gep->getPointerOperand();
      } else if (auto *ptr_as_const_gep = dyn_cast<llvm::ConstantExpr>(ptr)) {
        global_ptr = ptr_as_const_gep->getOperand(0);
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

  // Mark the variable for cleanup.
  this->markForCleanup(I);

  return;
}

// Logistics implementation.
void SSADestructionVisitor::cleanup() {
  for (auto *inst : this->instructions_to_delete) {
    infoln(*inst);

    if (not inst->hasNUses(0)) {
      for (auto *user : inst->users()) {
        if (auto *user_as_inst = dyn_cast_or_null<llvm::Instruction>(user)) {
          this->instructions_to_delete.insert(user_as_inst);
        }
      }
    }

    inst->replaceAllUsesWith(nullptr);

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

  this->replaced_values[&V] = replacement;
}

void SSADestructionVisitor::markForCleanup(MemOIRInst &I) {
  this->markForCleanup(I.getCallInst());
}

void SSADestructionVisitor::markForCleanup(llvm::Instruction &I) {
  this->instructions_to_delete.insert(&I);
}

} // namespace llvm::memoir
