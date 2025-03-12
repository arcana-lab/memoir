#include <string>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

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

static FunctionCallee get_function_callee(llvm::Module &M, std::string name) {
  auto *function = M.getFunction(name);
  if (not function) {
    MEMOIR_UNREACHABLE("Couldn't find ", name);
  }

  return FunctionCallee(function);
}

static FunctionCallee prepare_call(MemOIRBuilder &builder,
                                   const std::string &prefix,
                                   const std::string &operation,
                                   vector<llvm::Value *> &arguments) {

  auto function_name = prefix + "__" + operation;

  auto callee = detail::get_function_callee(builder.getModule(), function_name);
  auto *function_type = callee.getFunctionType();

  auto param_index = 0;
  for (auto &arg : arguments) {

    // Prepare the argument.
    MEMOIR_ASSERT(param_index < function_type->getNumParams(),
                  "Parameter ",
                  param_index,
                  " out of bounds in ",
                  function_name);
    auto *param_type = function_type->getParamType(param_index++);

    // If the argument doesn't match the parameter's type, prepare it.
    auto *arg_type = arg->getType();
    if (arg_type != param_type) {
      if (isa<llvm::IntegerType>(param_type)
          and isa<llvm::IntegerType>(arg_type)) {
        arg = builder.CreateZExtOrTrunc(arg, param_type);
      } else {
        println(*builder.GetInsertBlock()->getParent());
        println();
        println("at ", *builder.GetInsertPoint());
        MEMOIR_UNREACHABLE("Unhandled type mismatch! ",
                           *arg_type,
                           " ≠ ",
                           *param_type,
                           " for param ",
                           param_index);
      }
    }
  }

  return callee;
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

  auto &instantiation = implementation.instantiate(type);

  auto prefix = instantiation.get_prefix();

  return prepare_call(builder, prefix, operation, arguments);
}

llvm::Value &construct_field_read(MemOIRBuilder &builder,
                                  StructType &type,
                                  llvm::Value &object,
                                  unsigned field_index) {

  auto prefix = "impl__" + type.getName();

  auto operation = "read_" + std::to_string(field_index);

  vector<llvm::Value *> arguments = { &object };

  auto callee = prepare_call(builder, prefix, operation, arguments);

  return MEMOIR_SANITIZE(builder.CreateCall(callee, arguments),
                         "Failed to construct call for field read.");
}

void construct_field_write(MemOIRBuilder &builder,
                           StructType &type,
                           llvm::Value &object,
                           unsigned field_index,
                           llvm::Value &value_to_write) {

  auto prefix = "impl__" + type.getName();

  auto operation = "write_" + std::to_string(field_index);

  vector<llvm::Value *> arguments = { &object, &value_to_write };

  auto callee = prepare_call(builder, prefix, operation, arguments);

  MEMOIR_SANITIZE(builder.CreateCall(callee, arguments),
                  "Failed to construct call for field write.");

  return;
}

llvm::Value &construct_field_get(MemOIRBuilder &builder,
                                 StructType &type,
                                 llvm::Value &object,
                                 unsigned field_index) {

  auto prefix = "impl__" + type.getName();

  auto operation = "get_" + std::to_string(field_index);

  vector<llvm::Value *> arguments = { &object };

  auto callee = prepare_call(builder, prefix, operation, arguments);

  return MEMOIR_SANITIZE(builder.CreateCall(callee, arguments),
                         "Failed to construct call for field read.");
}

llvm::CallBase &construct_collection_write(
    MemOIRBuilder &builder,
    llvm::Value &object,
    CollectionType &collection_type,
    std::input_iterator auto index_begin,
    std::input_iterator auto index_end,
    llvm::Value &written,
    const Implementation &implementation) {

  vector<llvm::Value *> arguments = { &object };
  arguments.insert(arguments.end(), index_begin, index_end);
  arguments.push_back(&written);

  auto callee = detail::prepare_call(builder,
                                     implementation,
                                     collection_type,
                                     "write",
                                     arguments);

  return MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                         "Failed to construct collection write.");
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

namespace detail {

llvm::Value &construct_collection_allocation(
    MemOIRBuilder &builder,
    CollectionType &type,
    llvm::ArrayRef<llvm::Value *> sizes,
    std::optional<SelectionMetadata> selection,
    unsigned selection_index = 0) {

  auto *collection_type = &type;

  auto size_it = sizes.begin(), size_ie = sizes.end();

  // Allocate the nested collections.
  llvm::Value *result = nullptr;
  llvm::Use *nesting_use = nullptr;
  while (collection_type) {

    // If we are not the outermost, nor need to be initialized then we are
    // done.
    if (result and not nesting_use) {
      break;
    }

    // Fetch the collection implementation.
    std::optional<std::string> dim_name = std::nullopt;
    if (selection.has_value()) {
      dim_name = selection->getImplementation(selection_index++);
    }

    const auto &dim_impl =
        ImplLinker::get_implementation(dim_name, *collection_type);

    // Fetch the arguments.
    vector<llvm::Value *> arguments = {};
    auto *nested_type = collection_type;
    auto dim_size_it = size_it;
    bool in_assoc = false;
    for (unsigned dim = 0; dim < dim_impl.num_dimensions(); ++dim) {
      if (dim_size_it != size_ie) {
        arguments.push_back(*dim_size_it);
        ++dim_size_it;
      } else if (not in_assoc and isa<SequenceType>(nested_type)) {
        arguments.push_back(builder.getInt64(0));
      } else if (isa<AssocType>(nested_type)) {
        in_assoc = true;
      }

      auto &elem_type = nested_type->getElementType();
      nested_type = dyn_cast_or_null<CollectionType>(&elem_type);
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

    // If the nested element is a collection and this collection is a
    // sequence, construct a for loop to initialize its elements.
    if (nested_type and isa<SequenceType>(collection_type)) {

      // For:
      // A = alloc(N, M)
      // Construct the loop:
      //   if (N != 0)
      //     for (i = 0; i < N; ++i)
      //       A[i] = { ... };

      // Construct the if conditions and for loops.
      vector<llvm::Value *> loop_indices = {};
      for (auto it = size_it; it != dim_size_it; ++it) {
        auto *init_size = *size_it;
        auto *zero_constant = Constant::getNullValue(init_size->getType());
        auto *size_is_nonzero = builder.CreateICmpNE(init_size, zero_constant);

        auto *then_body =
            llvm::SplitBlockAndInsertIfThen(size_is_nonzero,
                                            builder.GetInsertPoint(),
                                            /* Unreachable? */ false);

        auto [loop_body, loop_iv] =
            llvm::SplitBlockAndInsertSimpleForLoop(init_size, then_body);

        // Update the insertion point for future construction.
        builder.SetInsertPoint(loop_body);

        loop_indices.push_back(loop_iv);
      }

      // Construct the write instruction with an undefined value being
      // written for the time being.
      auto &undef =
          MEMOIR_SANITIZE(llvm::UndefValue::get(
                              llvm::PointerType::get(builder.getContext(), 0)),
                          "Failed to get undef!");
      auto &write = detail::construct_collection_write(builder,
                                                       call,
                                                       *collection_type,
                                                       loop_indices.begin(),
                                                       loop_indices.end(),
                                                       undef,
                                                       dim_impl);

      builder.SetInsertPoint(&write);

      // Save the Use that needs updated by the actual allocation.
      for (auto &operand : write.args()) {
        if (operand.get() == &undef) {
          nesting_use = &operand;
          break;
        }
      }
    }

    if (not result) {
      result = &call;
    } else if (nesting_use) {
      nesting_use->set(&call);
      nesting_use = nullptr;
    } else {
      MEMOIR_UNREACHABLE(
          "Malformed allocation, initializing nested collection w/o a nesting collection.");
    }

    size_it = dim_size_it;
    collection_type = nested_type;
  }

  return *result;
}
} // namespace detail

void SSADestructionVisitor::visitAllocInst(AllocInst &I) {

  MemOIRBuilder builder(I);

  auto *type = &I.getType();

  if (auto *collection_type = dyn_cast<CollectionType>(type)) {

    // Fetch the selection from the instruction metadata, if it exists.
    unsigned selection_index = 0;
    auto selection_metadata = Metadata::get<SelectionMetadata>(I);

    // Track where we are in the size list.
    auto size_it = I.sizes_begin(), size_ie = I.sizes_end();

    // Construct the allocation.
    auto &result = detail::construct_collection_allocation(
        builder,
        *collection_type,
        llvm::SmallVector<llvm::Value *>(size_it, size_ie),
        selection_metadata);

    this->coalesce(I, result);

  } else if (auto *struct_type = dyn_cast<StructType>(type)) {

    auto prefix = "impl__" + struct_type->getName();

    auto operation = "allocate";

    vector<llvm::Value *> arguments = {};

    auto callee = detail::prepare_call(builder, prefix, operation, arguments);

    auto &call = MEMOIR_SANITIZE(builder.CreateCall(callee, arguments),
                                 "Couldn't create call for struct allocation!");

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
    auto selection_metadata = Metadata::get<SelectionMetadata>(I);
    if (selection_metadata.has_value()) {
      impl_name = selection_metadata->getImplementation();
    }

    const auto &impl =
        ImplLinker::get_implementation(impl_name, *collection_type);

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
static llvm::Value &contextualize_end(AccessInst &inst, llvm::Use &use) {

  bool minus_one =
      (isa<ReadInst>(&inst) or isa<WriteInst>(&inst) or isa<GetInst>(&inst)
       or isa<SizeInst>(&inst) or isa<RemoveInst>(&inst));

  if (std::next(&use) < inst.index_operands_end()) {
    minus_one = true;
  }

  if (auto range_kw = inst.get_keyword<RangeKeyword>()) {
    if (use == range_kw->getBeginAsUse() or use == range_kw->getEndAsUse()) {
      minus_one = false;
    }
  }

  vector<llvm::Value *> indices = {};
  for (auto &index_use : inst.index_operands()) {
    if (&use == &index_use) {
      break;
    }
    indices.push_back(index_use.get());
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
      auto &contextualized = contextualize_end(*access, use);
      if (auto *contextualized_inst =
              dyn_cast<llvm::Instruction>(&contextualized)) {
        this->stage(*contextualized_inst);
      }
    } else if (auto *phi_node = dyn_cast<llvm::PHINode>(user_as_inst)) {
      MEMOIR_UNREACHABLE(
          "Contextualizing EndInst at a PHINode is not yet supported!");
    } else if (auto *call = dyn_cast<llvm::CallBase>(user_as_inst)) {
      MEMOIR_UNREACHABLE(
          "Contextualizing EndInst intraprocedurally is not yet supported!\n",
          "  in ",
          *call);
    } else {
      MEMOIR_UNREACHABLE("Unknown user of EndInst: ", *user_as_inst);
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
                   const Implementation &impl,
                   unsigned selection_index)
    : object(object),
      type(type),
      begin(begin),
      end(end),
      implementation(&impl),
      selection_index(selection_index) {}

  NestedObjectInfo(llvm::Value &object,
                   Type &type,
                   AccessInst::index_iterator begin,
                   AccessInst::index_iterator end)
    : object(object),
      type(type),
      begin(begin),
      end(end),
      implementation(nullptr),
      selection_index(0) {}

  llvm::Value &object;
  Type &type;
  AccessInst::index_iterator begin, end;
  const Implementation *implementation;
  unsigned selection_index;
};
static NestedObjectInfo get_nested_object(
    llvm::Instruction &IP,
    llvm::Value &outer_object,
    Type &object_type,
    std::input_iterator auto indices_begin,
    std::input_iterator auto indices_end,
    TypeConverter &TC,
    bool fully_qualified = false,
    std::optional<SelectionMetadata> selection_metadata = {},
    unsigned selection_index = 0) {

  MemOIRBuilder builder(&IP);

  // Unpack the instruction.
  auto *object = &outer_object;
  auto *type = &object_type;

  // Fetch the module.
  auto &M = builder.getModule();

  // Construct nested access.
  for (auto it = indices_begin, ie = indices_end; it != ie;) {

    if (auto *struct_type = dyn_cast<StructType>(type)) {

      // Determine the field being accessed.
      auto *index = *it;
      auto &index_constant =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                          "Field index is not a constant integer.");
      auto field_index = index_constant.getZExtValue();
      auto &field_type = struct_type->getFieldType(field_index);

      // If the field type is a primitive, we have reached the innermost
      // object.
      if (not isa<ObjectType>(&field_type)) {
        return NestedObjectInfo(*object, *type, it, ie);
      }

      // Fetch the field's selection metadata.
      selection_metadata =
          Metadata::get<SelectionMetadata>(*struct_type, field_index);
      selection_index = 0;

      // Construct a get.
      object = &detail::construct_field_get(builder,
                                            *struct_type,
                                            *object,
                                            field_index);

      type = &field_type;
      it = std::next(it);

    } else if (auto *array_type = dyn_cast<ArrayType>(type)) {

      // Unpack the type.
      auto &element_type = array_type->getElementType();
      auto length = array_type->getLength();

      // If the element is a primitive, we have reached the innermost
      // object.
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
      auto *index = *it++;
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
          ImplLinker::get_implementation(dim_name, *collection_type);

      // Check if the implementation covers the remaining indices or not.
      auto num_dimensions = dim_impl.num_dimensions();
      auto remaining = std::distance(it, ie);
      if ((fully_qualified and (num_dimensions > remaining))
          or (not fully_qualified and (num_dimensions >= remaining))) {
        return NestedObjectInfo(*object,
                                *type,
                                it,
                                ie,
                                dim_impl,
                                selection_index);
      }

      // Otherwise, we will construct the get operation for the nested
      // object.
      vector<llvm::Value *> arguments = { object };
      auto dim_it = std::next(it, dim_impl.num_dimensions());
      arguments.insert(arguments.end(), it, dim_it);
      it = dim_it;

      auto &element_type = collection_type->getElementType();

      // Determine the type of operation based on the nested element type.
      auto operation = Type::is_unsized(element_type) ? "read" : "get";

      auto callee = detail::prepare_call(builder,
                                         dim_impl,
                                         *collection_type,
                                         operation,
                                         arguments);

      auto &call =
          MEMOIR_SANITIZE(builder.CreateCall(callee, llvm::ArrayRef(arguments)),
                          "Could not create the call for get");

      object = &call;
      type = &element_type;

    } else {
      MEMOIR_UNREACHABLE("Dimension mismatch!\n", "  ", IP);
    }
  }

  if (isa<StructType>(type) or isa<ArrayType>(type)) {
    return NestedObjectInfo(*object, *type, indices_end, indices_end);

  } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
    std::optional<std::string> impl_name = std::nullopt;
    if (selection_metadata.has_value()) {
      impl_name = selection_metadata->getImplementation(selection_index++);
    }
    const auto &impl =
        ImplLinker::get_implementation(impl_name, *collection_type);

    return NestedObjectInfo(*object,
                            *type,
                            indices_end,
                            indices_end,
                            impl,
                            selection_index);
  }

  MEMOIR_UNREACHABLE("Could not get the nested object\n  ",
                     IP,
                     "\n   in ",
                     IP.getFunction()->getName());
}

static NestedObjectInfo get_nested_object(AccessInst &I,
                                          TypeConverter &TC,
                                          bool fully_qualified = false) {
  return get_nested_object(I.getCallInst(),
                           I.getObject(),
                           I.getObjectType(),
                           I.indices_begin(),
                           I.indices_end(),
                           TC,
                           fully_qualified,
                           Metadata::get<SelectionMetadata>(I),
                           0);
}

unsigned compute_depth(NestedObjectInfo &info) {
  if (not info.implementation) {
    return 0;
  }

  return info.implementation->num_dimensions()
         - std::distance(info.begin, info.end);
}

} // namespace detail

void SSADestructionVisitor::visitReadInst(ReadInst &I) {

  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC);

  // Construct the read.
  MemOIRBuilder builder(I);

  llvm::Value *result = nullptr;
  if (auto *struct_type = dyn_cast<StructType>(&info.type)) {
    // There will only ever be a single index for an innermost struct
    // access.
    auto &field_value = MEMOIR_SANITIZE(*info.begin, "Field index is NULL!");
    auto &field_const =
        MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(&field_value),
                        "Field index is not statically known!");
    auto field_index = field_const.getZExtValue();

    // Fetch the type layout for the struct.
    auto &layout = TC.convert(*struct_type);

    // Construct the read.
    result = &detail::construct_field_read(builder,
                                           *struct_type,
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
  auto info = detail::get_nested_object(I, this->TC);

  // Construct the read.
  MemOIRBuilder builder(I);

  if (auto *struct_type = dyn_cast<StructType>(&info.type)) {
    // There will only ever be a single index for an innermost struct
    // access.
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
    detail::construct_collection_write(builder,
                                       info.object,
                                       *collection_type,
                                       info.begin,
                                       info.end,
                                       I.getValueWritten(),
                                       *info.implementation);
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
  auto info = detail::get_nested_object(I, this->TC);

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

  // Construct the read.
  MemOIRBuilder builder(I);

  // Handle operation keywords.
  vector<llvm::Value *> arguments = {};

  bool base_operation = true;
  bool fully_qualified = false;
  std::string operation_name = "insert";
  if (auto value_kw = I.get_keyword<ValueKeyword>()) {
    operation_name += "_value";
    arguments.push_back(&value_kw->getValue());
    base_operation = false;

  } else if (auto input_kw = I.get_keyword<InputKeyword>()) {
    // IDEA: extend this to support set and map unions?
    operation_name += "_input";
    fully_qualified = isa<AssocType>(I.getObjectType());
    base_operation = false;

    std::optional<SelectionMetadata> input_selection = {};
    if (auto *input_inst = dyn_cast<llvm::Instruction>(&input_kw->getInput())) {
      input_selection = Metadata::get<SelectionMetadata>(*input_inst);
    }

    auto &input_type =
        MEMOIR_SANITIZE(type_of(input_kw->getInput()),
                        "Failed to get type of InputKeyword object");

    auto input_info = detail::get_nested_object(I.getCallInst(),
                                                input_kw->getInput(),
                                                input_type,
                                                input_kw->indices_begin(),
                                                input_kw->indices_end(),
                                                this->TC,
                                                /* fully qualified? */ true,
                                                input_selection);

    arguments.push_back(&input_info.object);

    if (auto range_kw = I.get_keyword<RangeKeyword>()) {
      operation_name += "_range";
      arguments.push_back(&range_kw->getBegin());
      arguments.push_back(&range_kw->getEnd());
    }
  }

  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, fully_qualified);

  // Compute the operation depth.
  auto depth = fully_qualified ? 0 : detail::compute_depth(info);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  auto &element_type = I.getElementType();

  // Fetch the function that implements this operation.
  arguments.insert(arguments.begin(), info.begin, info.end);
  arguments.insert(arguments.begin(), &info.object);

  if (base_operation and depth == 0 and Type::is_unsized(element_type)) {
    // If the element type is unsized, we must provide the default
    // initializer.
    if (auto *nested_collection_type =
            dyn_cast<CollectionType>(&element_type)) {
      // Default initialize the nested collection.

      vector<llvm::Value *> args = {};
      if (isa<SequenceType>(nested_collection_type)) {
        args.push_back(builder.getInt64(0));
      }

      // TODO: How do we get the nested implementation here?
      auto *alloc = builder.CreateAllocInst(element_type, args, "default");
      this->stage(alloc->getCallInst());

      // Propagate the nested selection info here.
      auto selection = Metadata::get<SelectionMetadata>(I);
      if (selection.has_value()) {
        auto alloc_selection = Metadata::get_or_add<SelectionMetadata>(*alloc);

        auto sel_it = selection->impl_begin();
        for (unsigned i = 0; i < info.selection_index; ++i) {
          if (sel_it != selection->impl_end()) {
            ++sel_it;
          }
        }

        unsigned alloc_index = 0;
        for (; sel_it != selection->impl_end(); ++sel_it, ++alloc_index) {
          auto sel = *sel_it;
          if (sel.has_value()) {
            alloc_selection.setImplementation(sel.value(), alloc_index);
          }
        }
      }

      operation_name += "_value";
      arguments.push_back(&alloc->getCallInst());

    } else {
      MEMOIR_UNREACHABLE(
          "Inserting an element type with unknown default initializer!");
    }
  }

  // If this is not a complete depth operation, append the depth.
  if (depth > 0) {
    operation_name += "__" + std::to_string(depth);
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
  this->coalesce(I, I.getObject());

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitRemoveInst(RemoveInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC);

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
  this->coalesce(I, I.getObject());

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitCopyInst(CopyInst &I) {
  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC);

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
  auto info = detail::get_nested_object(I, this->TC, true);

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
  this->coalesce(I, I.getObject());

  // The instruction is dead now.
  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitSizeInst(SizeInst &I) {

  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, true);

  // Compute the operation depth.
  auto depth = detail::compute_depth(info);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                      "Trying to get size of non-collection object\n  ",
                      I,
                      "\n   in ",
                      I.getFunction()->getName());

  // Fetch the function that implements this operation.
  vector<llvm::Value *> arguments = { &info.object };
  arguments.insert(arguments.end(), info.begin, info.end);

  std::string operation = "size";

  if (depth > 0) {
    operation += "__" + std::to_string(depth);
  }

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
  auto info = detail::get_nested_object(I, this->TC);

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
void SSADestructionVisitor::visitFoldInst(FoldInst &I) {

  // Get the nested object as a value.
  auto info = detail::get_nested_object(I, this->TC, true);

  // Construct the read.
  MemOIRBuilder builder(I);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&info.type),
                                          "Non-collection object to ",
                                          I);

  // Get the functions to call.
  auto &instantiation = info.implementation->instantiate(collection_type);

  auto prefix = instantiation.get_prefix();

  auto begin_name = prefix + (I.isReverse() ? "__rbegin" : "__begin");
  auto next_name = prefix + (I.isReverse() ? "__rnext" : "__next");

  auto begin_callee = detail::get_function_callee(this->M, begin_name);
  auto next_callee = detail::get_function_callee(this->M, next_name);

  // Unpack the functions.
  auto *begin_func = cast<llvm::Function>(begin_callee.getCallee());
  auto *next_func = cast<llvm::Function>(next_callee.getCallee());

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
      info.object,
      collection_type,
      begin_func,
      next_func,
      iter_type,
      [&](llvm::Value &orig, llvm::Value &replacement) {
        this->coalesce(orig, replacement);
      },
      [&](llvm::Instruction &I) { this->markForCleanup(I); });

  this->markForCleanup(I);

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

  if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    if (auto *new_inst = dyn_cast<llvm::Instruction>(&V)) {
      new_inst->cloneDebugInfoFrom(inst);
    }
  }
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

const set<llvm::Instruction *> &SSADestructionVisitor::staged() {
  return this->_staged;
}

void SSADestructionVisitor::stage(llvm::Instruction &I) {
  this->_staged.insert(&I);
}

void SSADestructionVisitor::clear_stage() {
  this->_staged.clear();
}

} // namespace llvm::memoir
