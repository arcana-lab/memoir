#include "llvm/IR/InstIterator.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/passes/Passes.hpp"
#include "memoir/support/Print.hpp"

namespace memoir {

static bool coerce(llvm::Use &use,
                   Type *dst_memoir_type,
                   llvm::Type *dst_type) {

  // Unpack the use.
  auto *user = use.getUser();
  auto *used = use.get();
  auto *src_type = used->getType();

  // Find the construction point for this use.
  auto *insertion_point = dyn_cast<llvm::Instruction>(user);
  if (auto *phi = dyn_cast<llvm::PHINode>(user)) {

    // Insert before the terminator of the incoming block.
    auto *block = phi->getIncomingBlock(use.getOperandNo());
    auto *terminator = block->getTerminator();
    insertion_point = terminator;
  }

  MEMOIR_ASSERT(insertion_point,
                "Failed to find insertion point for use ",
                pretty(use));

  MemOIRBuilder builder(insertion_point);

  // Determine the correct cast operation.
  auto *src_int_type = dyn_cast<llvm::IntegerType>(src_type);
  auto *dst_int_type = dyn_cast<llvm::IntegerType>(dst_type);

  // Determine the signedness of the operation, if relevant.
  bool is_signed = false;
  if (auto *dst_int_memoir = dyn_cast<IntegerType>(dst_memoir_type)) {
    is_signed = dst_int_memoir->isSigned();
  }

  // Handle integer-to-integer casts.
  if (src_int_type and dst_int_type) {
    // Create extension or truncate.
    llvm::Value *coerced = NULL;
    if (is_signed) {
      coerced = builder.CreateSExtOrTrunc(used, dst_int_type, "coerce.");
    } else {
      coerced = builder.CreateZExtOrTrunc(used, dst_int_type, "coerce.");
    }

    use.set(coerced);
    return true;
  }

  // TODO: Handle float-to-integer casts.

  // TODO: handle integer-to-float casts.

  // TODO: Handle arbitrary bitcasts.

  return false;
}

static bool coerce(llvm::Use &use, Type &dst_type) {
  // Convert the memoir type to an LLVM type.
  auto *user = dyn_cast<llvm::Instruction>(use.getUser());
  auto *module = user->getModule();
  auto &context = module->getContext();
  auto *dst_llvm_type = dst_type.get_llvm_type(context);

  return coerce(use, &dst_type, dst_llvm_type);
}

static bool coerce(AccessInst &access) {
  bool modified = false;

  // Fetch the object type.
  auto *obj_type = &access.getObjectType();

  // Fetch module and context information.
  auto *module = access.getModule();
  if (not module) {
    return false;
  }
  auto &context = module->getContext();
  auto &data_layout = module->getDataLayout();

  for (auto &index_use : access.index_operands()) {
    auto *index = index_use.get();

    if (auto *collection_type = dyn_cast<CollectionType>(obj_type)) {

      Type *correct_type = NULL;
      if (auto *assoc_type = dyn_cast<AssocType>(collection_type)) {
        correct_type = &assoc_type->getKeyType();

      } else if (isa<SequenceType>(collection_type)
                 or isa<ArrayType>(collection_type)) {
        correct_type = &Type::get_size_type(data_layout);
      } else {
        MEMOIR_UNREACHABLE("Unknown collection type ", *collection_type);
      }

      auto *correct_llvm_type = correct_type->get_llvm_type(context);
      auto *index_type = index->getType();

      // If the index doesn't match the correct type, we need to coerce it.
      if (correct_llvm_type != index_type) {
        // Coerce the index use to the correct type.
        modified |= coerce(index_use, correct_type, correct_llvm_type);
      }

    } else if (auto *tuple_type = dyn_cast<TupleType>(obj_type)) {

      auto *field_const = dyn_cast<llvm::ConstantInt>(index);
      MEMOIR_ASSERT(field_const, "Field offset is not a constant!");
      auto field = field_const->getZExtValue();

      obj_type = &tuple_type->getFieldType(field);
      continue;

    } else {
      MEMOIR_UNREACHABLE("Unknown object type ", *obj_type);
    }
  }

  return modified;
}

static bool coerce(llvm::User &user) {
  bool modified = false;

  // Only handle memoir instructions.
  auto *memoir = into<MemOIRInst>(&user);
  if (not memoir) {
    return modified;
  }

  // Handle the indices of access instructions.
  if (auto *access = dyn_cast<AccessInst>(memoir)) {
    modified |= coerce(*access);

    // Handle write values.
    if (auto *write = dyn_cast<WriteInst>(memoir)) {
      modified |=
          coerce(write->getValueWrittenAsUse(), write->getElementType());
    }

    // Handle value keywords.
    if (auto value_kw = access->get_keyword<ValueKeyword>()) {
      modified |= coerce(value_kw->getValueAsUse(), access->getElementType());
    }
  }

  return modified;
}

llvm::PreservedAnalyses CoercePass::run(llvm::Function &F,
                                        llvm::FunctionAnalysisManager &FAM) {
  bool modified = false;

  for (auto &I : llvm::instructions(F)) {
    modified |= coerce(I);
  }

  return modified ? llvm::PreservedAnalyses::all()
                  : llvm::PreservedAnalyses::none();
}

} // namespace memoir
