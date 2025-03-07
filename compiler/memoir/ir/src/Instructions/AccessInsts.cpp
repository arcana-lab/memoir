#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// AccessInst implementation
Type &AccessInst::getObjectType() const {
  auto *type = type_of(this->getObject());

  if (not type) {
    if (const auto &debug_loc = this->getCallInst().getDebugLoc()) {
      print("DEBUG INFO: ");
      debug_loc.print(llvm::errs());
    }

    MEMOIR_UNREACHABLE("Could not determine type of object being accessed!\n  ",
                       *this,
                       "\n  in ",
                       this->getFunction()->getName());
  }

  return *type;
}

Type &AccessInst::getElementType() const {
  // Determine the element type from the operation.
  switch (this->getKind()) {
    case MemOIR_Func::READ_UINT64:
    case MemOIR_Func::WRITE_UINT64:
    case MemOIR_Func::MUT_WRITE_UINT64:
      return Type::get_u64_type();
    case MemOIR_Func::READ_UINT32:
    case MemOIR_Func::WRITE_UINT32:
    case MemOIR_Func::MUT_WRITE_UINT32:
      return Type::get_u32_type();
    case MemOIR_Func::READ_UINT16:
    case MemOIR_Func::WRITE_UINT16:
    case MemOIR_Func::MUT_WRITE_UINT16:
      return Type::get_u16_type();
    case MemOIR_Func::READ_UINT8:
    case MemOIR_Func::WRITE_UINT8:
    case MemOIR_Func::MUT_WRITE_UINT8:
      return Type::get_u8_type();
    case MemOIR_Func::READ_INT64:
    case MemOIR_Func::WRITE_INT64:
    case MemOIR_Func::MUT_WRITE_INT64:
      return Type::get_i64_type();
    case MemOIR_Func::READ_INT32:
    case MemOIR_Func::WRITE_INT32:
    case MemOIR_Func::MUT_WRITE_INT32:
      return Type::get_i32_type();
    case MemOIR_Func::READ_INT16:
    case MemOIR_Func::WRITE_INT16:
    case MemOIR_Func::MUT_WRITE_INT16:
      return Type::get_i16_type();
    case MemOIR_Func::READ_INT8:
    case MemOIR_Func::WRITE_INT8:
    case MemOIR_Func::MUT_WRITE_INT8:
      return Type::get_i8_type();
    case MemOIR_Func::READ_INT2:
    case MemOIR_Func::WRITE_INT2:
    case MemOIR_Func::MUT_WRITE_INT2:
      return Type::get_i2_type();
    case MemOIR_Func::READ_BOOL:
    case MemOIR_Func::WRITE_BOOL:
    case MemOIR_Func::MUT_WRITE_BOOL:
      return Type::get_bool_type();
    case MemOIR_Func::READ_DOUBLE:
    case MemOIR_Func::WRITE_DOUBLE:
    case MemOIR_Func::MUT_WRITE_DOUBLE:
      return Type::get_f64_type();
    case MemOIR_Func::READ_FLOAT:
    case MemOIR_Func::WRITE_FLOAT:
    case MemOIR_Func::MUT_WRITE_FLOAT:
      return Type::get_f32_type();
    case MemOIR_Func::READ_PTR:
    case MemOIR_Func::WRITE_PTR:
    case MemOIR_Func::MUT_WRITE_PTR:
      return Type::get_ptr_type();
    default: { // Otherwise, analyze the function to determine the type.
      // Get the collection type.
      auto *type = &this->getObjectType();

      for (auto *index : this->indices()) {
        if (auto *struct_type = dyn_cast<StructType>(type)) {
          auto &index_constant =
              MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                              "Struct field index is not constant.\n  ",
                              *index,
                              " in ",
                              *this);
          auto index_value = index_constant.getZExtValue();
          type = &struct_type->getFieldType(index_value);

        } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
          type = &collection_type->getElementType();
        }
      }

      return MEMOIR_SANITIZE(type,
                             "Couldn't determine type of accessed element.");
    }
  }
}

llvm::iterator_range<AccessInst::index_iterator> AccessInst::indices() {
  return llvm::make_range(this->indices_begin(), this->indices_end());
}

AccessInst::index_iterator AccessInst::indices_begin() {
  return index_iterator(this->index_operands_begin());
}

AccessInst::index_iterator AccessInst::indices_end() {
  return index_iterator(this->index_operands_end());
}

llvm::iterator_range<AccessInst::const_index_iterator> AccessInst::indices()
    const {
  return llvm::make_range(this->indices_begin(), this->indices_end());
}

AccessInst::const_index_iterator AccessInst::indices_begin() const {
  return const_index_iterator(this->index_operands_begin());
}

AccessInst::const_index_iterator AccessInst::indices_end() const {
  return const_index_iterator(this->index_operands_end());
}

llvm::iterator_range<AccessInst::index_op_iterator> AccessInst::
    index_operands() {
  return llvm::make_range(this->index_operands_begin(),
                          this->index_operands_end());
}

AccessInst::index_op_iterator AccessInst::index_operands_begin() {
  return index_op_iterator(std::next(&this->getObjectAsUse()));
}

AccessInst::index_op_iterator AccessInst::index_operands_end() {
  return index_op_iterator(this->kw_begin().asUse());
}

llvm::iterator_range<AccessInst::const_index_op_iterator> AccessInst::
    index_operands() const {
  return llvm::make_range(this->index_operands_begin(),
                          this->index_operands_end());
}

AccessInst::const_index_op_iterator AccessInst::index_operands_begin() const {
  return const_index_op_iterator(std::next(&this->getObjectAsUse()));
}

AccessInst::const_index_op_iterator AccessInst::index_operands_end() const {
  return const_index_op_iterator(this->kw_begin().asUse());
}

// ReadInst implementation
RESULTANT(ReadInst, ValueRead)
OPERAND(ReadInst, Object, 0)
TO_STRING(ReadInst)

// GetInst implementation
RESULTANT(GetInst, NestedObject)
OPERAND(GetInst, Object, 0)
TO_STRING(GetInst)

// CopyInst implementation
RESULTANT(CopyInst, Result)
OPERAND(CopyInst, Object, 0)
TO_STRING(CopyInst)

// SizeInst implementation
RESULTANT(SizeInst, Size)
OPERAND(SizeInst, Object, 0)
TO_STRING(SizeInst)

// HasInst implementation
RESULTANT(HasInst, Result)
OPERAND(HasInst, Object, 0)
TO_STRING(HasInst)

// AssocKeysInst implementation.
RESULTANT(KeysInst, Result)
OPERAND(KeysInst, Object, 0)
TO_STRING(KeysInst)

// UpdateInst implementation
RESULTANT(UpdateInst, Result)

// WriteInst implementation
OPERAND(WriteInst, ValueWritten, 0)
OPERAND(WriteInst, Object, 1)
TO_STRING(WriteInst)

// InsertInst implementation
OPERAND(InsertInst, Object, 0)
TO_STRING(InsertInst)

// RemoveInst implementation
OPERAND(RemoveInst, Object, 0)
TO_STRING(RemoveInst)

// ClearInst implementation
OPERAND(ClearInst, Object, 0)
TO_STRING(ClearInst)

} // namespace llvm::memoir
