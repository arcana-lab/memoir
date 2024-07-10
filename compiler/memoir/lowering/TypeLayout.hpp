#ifndef MEMOIR_TYPECONVERTER_H
#define MEMOIR_TYPECONVERTER_H
#pragma once

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/ir/TypeVisitor.hpp"
#include "memoir/ir/Types.hpp"

/*
 * This file provides a type layout engine with which MEMOIR objects are lowered
 * to LLVM.
 *
 * Author(s): Tommy McMichen
 * Created: September 25, 2023
 */

namespace llvm::memoir {

#define CHECK_MEMOIZED(T)                                                      \
  auto found_type = this->memoir_to_type_layout.find(&T);                      \
  if (found_type != this->memoir_to_type_layout.end()) {                       \
    return *(found_type->second);                                              \
  }

#define MEMOIZE_AND_RETURN(T, LAYOUT)                                          \
  this->memoir_to_type_layout[&T] = &LAYOUT;                                   \
  return LAYOUT

// NOTE: this could be converted into a hierarchy, but I don't see the
// complexity reaching that level of necessity nor the storage of these objects
// to be big enough of an issue to warrant RTTI overhead.
struct TypeLayout {
public:
  TypeLayout(Type &memoir_type, llvm::Type &llvm_type)
    : memoir_type(memoir_type),
      llvm_type(llvm_type) {}
  TypeLayout(IntegerType &memoir_integer_type,
             llvm::IntegerType &llvm_integer_type,
             map<unsigned, pair<unsigned, unsigned>> bit_field_ranges)
    : memoir_type(memoir_integer_type),
      llvm_type(llvm_integer_type),
      bit_field_ranges(bit_field_ranges) {}
  TypeLayout(StructType &memoir_struct_type,
             llvm::StructType &llvm_struct_type,
             vector<unsigned> field_offsets)
    : memoir_type(memoir_struct_type),
      llvm_type(llvm_struct_type),
      field_offsets(field_offsets) {}
  TypeLayout(StructType &memoir_struct_type,
             llvm::StructType &llvm_struct_type,
             vector<unsigned> field_offsets,
             map<unsigned, pair<unsigned, unsigned>> bit_field_ranges)
    : memoir_type(memoir_struct_type),
      llvm_type(llvm_struct_type),
      field_offsets(field_offsets),
      bit_field_ranges(bit_field_ranges) {}

  Type &get_memoir_type() const {
    return memoir_type;
  }

  llvm::Type &get_llvm_type() const {
    return llvm_type;
  }

  bool has_fields() const {
    return !(this->field_offsets.empty());
  }

  unsigned get_field_offset(unsigned field_index) const {
    MEMOIR_ASSERT((field_index < field_offsets.size()),
                  "Field index out of range for offsets.");
    return field_offsets.at(field_index);
  }

  bool is_bit_field(unsigned field_index) const {
    return (this->bit_field_ranges.count(field_index) > 0);
  }

  opt<pair<unsigned, unsigned>> get_bit_field_range(unsigned field_index) {
    auto found_bit_field = this->bit_field_ranges.find(field_index);

    // If we found the field index, return its bit field range.
    if (found_bit_field != this->bit_field_ranges.end()) {
      return found_bit_field->second;
    }

    // Otherwise, return none.
    return {};
  }

protected:
  Type &memoir_type;
  llvm::Type &llvm_type;
  vector<unsigned> field_offsets;
  map<unsigned, pair<unsigned, unsigned>> bit_field_ranges;
}; // struct TypeLayout

class TypeConverter : public TypeVisitor<TypeConverter, TypeLayout &> {
  friend class TypeVisitor<TypeConverter, TypeLayout &>;

public:
  // Construction.
  TypeConverter(llvm::LLVMContext &C) : TypeVisitor(), C(C) {}

  TypeLayout &convert(Type &T) {
    return this->visit(T);
  }

protected:
  // Singleton.
  TypeConverter(TypeConverter &) = delete;
  TypeConverter &operator=(TypeConverter &&) = delete;

  TypeLayout &visitType(Type &T) {
    MEMOIR_UNREACHABLE("Unhandled type!");
  }

  static bool is_size(unsigned n) {
    switch (n) {
      case 8:
      case 16:
      case 32:
      case 64:
      case 128:
        return true;
      default:
        return false;
    }
  }

  // Takes a bitwidth (n) and returns the next power of 2, byte-aligned,
  // bitwidth.
  static unsigned get_next_size(unsigned n) {
    if (n < 8) {
      return 8;
    }
    unsigned m = (n >> 3) - 1;
    m |= m >> 1;
    m |= m >> 2;
    m |= m >> 4;
    m++;
    return m << 3;
  }

  TypeLayout &visitIntegerType(IntegerType &T) {
    CHECK_MEMOIZED(T);

    // Get integer type information.
    auto bitwidth = T.getBitWidth();

    // Get the power-of-2 byte aligned width.
    auto size_for_bitwidth = get_next_size(bitwidth);

    // Get the LLVM type.
    auto &llvm_type = MEMOIR_SANITIZE(llvm::IntegerType::get(this->C, bitwidth),
                                      "Could not get the LLVM IntegerType");

    // Create the type layout.
    auto *type_layout =
        (bitwidth == size_for_bitwidth)
            ? new TypeLayout(T, llvm_type)
            : new TypeLayout(T, llvm_type, { { 0, { 0, bitwidth } } });

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitFloatType(FloatType &T) {
    CHECK_MEMOIZED(T);

    // Get the LLVM type.
    auto &llvm_type = MEMOIR_SANITIZE(llvm::Type::getFloatTy(this->C),
                                      "Could not get the LLVM float type");

    // Create the type layout.
    auto *type_layout = new TypeLayout(T, llvm_type);

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitDoubleType(DoubleType &T) {
    CHECK_MEMOIZED(T);

    // Get the LLVM type.
    auto &llvm_type = MEMOIR_SANITIZE(llvm::Type::getDoubleTy(this->C),
                                      "Could not get the LLVM double type");

    // Create the type layout.
    auto *type_layout = new TypeLayout(T, llvm_type);

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitPointerType(PointerType &T) {
    CHECK_MEMOIZED(T);

    // Get the LLVM type.
    auto *llvm_void_type = llvm::Type::getInt8Ty(this->C);
    auto &llvm_type = MEMOIR_SANITIZE(llvm::PointerType::get(llvm_void_type, 0),
                                      "Could not get the LLVM void ptr type");

    // Create the type layout.
    auto *type_layout = new TypeLayout(T, llvm_type);

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitVoidType(VoidType &T) {
    CHECK_MEMOIZED(T);

    // Get the LLVM type.
    auto &llvm_type = MEMOIR_SANITIZE(llvm::Type::getVoidTy(this->C),
                                      "Could not get the LLVM void type");

    // Create the type layout.
    auto *type_layout = new TypeLayout(T, llvm_type);

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitReferenceType(ReferenceType &T) {
    CHECK_MEMOIZED(T);

    // Get the type layout of the referenced type.
    auto &referenced_type = this->visit(T.getReferencedType());

    // Create the pointer type.
    auto &llvm_type = MEMOIR_SANITIZE(
        llvm::PointerType::get(&referenced_type.get_llvm_type(), 0),
        "Could not construct the llvm PointerType for ReferenceType!");

    // Create the type layout.
    auto *type_layout = new TypeLayout(T, llvm_type);

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitStaticTensorType(StaticTensorType &T) {
    CHECK_MEMOIZED(T);

    // Get the type layout of the element.
    auto &element_type = this->visit(T.getElementType());

    // Get the dimension information.
    auto num_dimensions = T.getNumberOfDimensions();
    MEMOIR_ASSERT(
        (num_dimensions == 1),
        "Support for multidimensional static tensor types is unsupported");
    auto num_elements = T.getLengthOfDimension(0);

    // Create the vector type.
    auto &llvm_type = MEMOIR_SANITIZE(
        llvm::ArrayType::get(&element_type.get_llvm_type(), num_elements),
        "Could not construct the llvm VectorType for StaticTensorType.");

    // Create the type layout.
    auto *type_layout = new TypeLayout(T, llvm_type);

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitTensorType(TensorType &T) {
    MEMOIR_UNREACHABLE("TensorType lowering is unimplemented.");
  }

  TypeLayout &visitFieldArrayType(FieldArrayType &T) {
    MEMOIR_UNREACHABLE("FieldArrayType lowering is unimplemented.");
  }

  TypeLayout &visitStructType(StructType &T) {
    CHECK_MEMOIZED(T);

    // Check if we already created the named type.
    auto type_name = T.getName();
    auto llvm_struct_type_name = "memoir." + type_name;

    // Collection information about the struct type.
    auto num_fields = T.getNumFields();

    // Construct the llvm StructType for the given type.
    vector<unsigned> field_offsets;
    field_offsets.resize(num_fields);
    vector<llvm::Type *> llvm_field_types;
    llvm_field_types.reserve(num_fields);
    map<unsigned, pair<unsigned, unsigned>> bit_field_ranges = {};

    // Get the type layout of each field.
    unsigned current_field_width = 0;
    unsigned current_field_offset = 0;
    for (unsigned field_index = 0; field_index < num_fields; ++field_index) {
      // Get the field type.
      auto &field_type = T.getFieldType(field_index);

      // Convert the field type to its type layout.
      auto &field_layout = this->visit(field_type);
      auto &llvm_field_type = field_layout.get_llvm_type();

      // If this is an integer type, and it is a non-power-of-2, figure out if
      // it needs a bit field representation.
      bool is_bit_field = false;
      if (auto *field_int_type =
              dyn_cast<llvm::IntegerType>(&llvm_field_type)) {
        if (field_layout.is_bit_field(0)) {
          // Write the bit field range.
          auto bit_field_width = (*field_layout.get_bit_field_range(0)).second;
          auto bit_field_start = current_field_width;
          current_field_width += bit_field_width;
          bit_field_ranges[field_index] =
              make_pair(bit_field_start, current_field_width);
          debugln("new bit field (",
                  bit_field_start,
                  ", ",
                  current_field_width,
                  ")");

          // Mark this as a bit field so that we don't increment the field
          // offset.
          is_bit_field = true;
        }
      }

      // If we were in a bit field, but no longer are, commit the previous bit
      // field before handling this one.
      // Get the integer type.
      if (!is_bit_field && current_field_width != 0) {
        // Get the integer type.
        auto *bit_field_int_type =
            llvm::IntegerType::get(this->C, current_field_width);

        // Append the bit field aggregate type.
        llvm_field_types.push_back(bit_field_int_type);

        // Reset the bit field information.
        current_field_width = 0;

        // Increment the field offset.
        ++current_field_offset;
      }

      // Record the field offset.
      field_offsets[field_index] = current_field_offset;

      // Handle bit fields.
      if (is_bit_field) {
        // If the new field offset is a size, then instantiate that field.
        if (is_size(current_field_width)) {
          // Get the integer type.
          auto *bit_field_int_type =
              llvm::IntegerType::get(this->C, current_field_width);

          // Append the bit field aggregate type.
          llvm_field_types.push_back(bit_field_int_type);
        }

        // Otherwise, continue.
        else {
          continue;
        }
      }

      // Reset the bit field information.
      current_field_width = 0;

      // Increment the field offset.
      ++current_field_offset;

      // Append the new field.
      llvm_field_types.push_back(&llvm_field_type);
    }

    if (current_field_width != 0) {
      // Get the integer type.
      auto bit_field_size = get_next_size(current_field_width);
      debugln(current_field_width, " ==> ", bit_field_size);
      auto *bit_field_int_type =
          llvm::IntegerType::get(this->C, bit_field_size);

      // Append the bit field aggregate type.
      llvm_field_types.push_back(bit_field_int_type);
    }

    // Create the LLVM struct type.
    auto &llvm_type = MEMOIR_SANITIZE(
        llvm::StructType::create(llvm::ArrayRef(llvm_field_types),
                                 llvm_struct_type_name,
                                 /* is packed? */ true),
        "Could not create the LLVM StructType!");

    // Create the type layout.
    auto *type_layout =
        new TypeLayout(T, llvm_type, field_offsets, bit_field_ranges);

    MEMOIZE_AND_RETURN(T, *type_layout);
  }

  TypeLayout &visitAssocArrayType(AssocArrayType &T) {
    MEMOIR_UNREACHABLE("AssocArrayType is unimplemented!");
    // CHECK_MEMOIZED(T);

    // MEMOIZE_AND_RETURN(T, llvm_type);
  }

  TypeLayout &visitSequenceType(SequenceType &T) {
    MEMOIR_UNREACHABLE("SequenceType is unimplemented!");
    // CHECK_MEMOIZED(T);

    // MEMOIZE_AND_RETURN(T, llvm_type);
  }

  // Owned state.
  map<Type *, TypeLayout *> memoir_to_type_layout;

  // Borrowed state.
  llvm::LLVMContext &C;

}; // namespace llvm::memoir

} // namespace llvm::memoir

#endif // MEMOIR_TYPECONVERTER_H
