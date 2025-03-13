#include "llvm/ADT/ArrayRef.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"

namespace llvm::memoir {

// We will use two special offset values to represent either the keys or
// elements at a given offset.
#define ELEMS -1
#define KEYS -2

static void type_differences(vector<vector<unsigned>> &differences,
                             llvm::ArrayRef<unsigned> offsets,
                             Type &base,
                             Type &other) {

  // If the types are the same, continue.
  if (base == other) {
    return;
  }

  // If both of the types are non-equal primitive types, add the difference.
  if (Type::is_primitive_type(base)) {
    MEMOIR_ASSERT(Type::is_primitive_type(other),
                  "Non-isomorphic type mutation! ",
                  base,
                  " => ",
                  other);

    differences.emplace_back(offsets.begin(), offsets.end());

    print("DIFFERENCE: ");
    for (const auto &offset : differences.back()) {
      print(offset, ", ");
    }
    println();
  }

  // If these are the same type kind, recurse on their inner types.
  if (base.getKind() == other.getKind()) {

    if (auto *base_tuple = dyn_cast<TupleType>(&base)) {
      auto *other_tuple = cast<TupleType>(&other);

      auto base_fields = base_tuple->getNumFields();
      auto other_fields = other_tuple->getNumFields();
      MEMOIR_ASSERT(base_fields == other_fields,
                    "Non-isomorphic type mutation! ",
                    base,
                    " => ",
                    other);

      vector<unsigned> nested_offsets(offsets.begin(), offsets.end());
      for (unsigned field = 0; field < base_fields; ++field) {
        nested_offsets.push_back(field);

        type_differences(differences,
                         nested_offsets,
                         base_tuple->getFieldType(field),
                         other_tuple->getFieldType(field));

        nested_offsets.pop_back();
      }

      return;

    } else if (auto *base_assoc = dyn_cast<AssocType>(&base)) {
      auto *other_assoc = cast<AssocType>(&other);

      // Check differences on the key.
      auto &base_key = base_assoc->getKeyType();
      auto &other_key = other_assoc->getKeyType();

      vector<unsigned> nested_offsets(offsets.begin(), offsets.end());

      nested_offsets.push_back(-2);
      type_differences(differences, nested_offsets, base_key, other_key);
      nested_offsets.pop_back();

      // Check differences on the element.
      auto &base_elem = base_assoc->getElementType();
      auto &other_elem = other_assoc->getElementType();

      nested_offsets.push_back(-1);
      type_differences(differences, nested_offsets, base_elem, other_elem);
      nested_offsets.pop_back();

      return;

    } else if (auto *base_seq = dyn_cast<SequenceType>(&base)) {
      auto *other_seq = cast<SequenceType>(&other);

      auto &base_elem = base_seq->getElementType();
      auto &other_elem = other_seq->getElementType();

      vector<unsigned> nested_offsets(offsets.begin(), offsets.end());

      nested_offsets.push_back(-1);
      type_differences(differences, nested_offsets, base_elem, other_elem);
      nested_offsets.pop_back();

      return;
    }
  }

  // If we are switching from assoc -> sequence

  // Or from sequence -> assoc

  // Otherwise, the mutation is not isomorphic, unhandled!
  // Do it yourself!
  MEMOIR_UNREACHABLE("Non-isomorphic type mutation! ", base, " => ", other);

  return;
}

static vector<vector<unsigned>> type_differences(Type &base, Type &other) {
  vector<vector<unsigned>> differences = {};
  vector<unsigned> offsets = {};

  println("TYPE DIFFERENCES");
  println("  ", base);
  println("  ", other);

  type_differences(differences, offsets, base, other);

  return differences;
}

map<llvm::Value *, llvm::Value *> mutate_type(AllocInst &alloc, Type &type) {
  map<llvm::Value *, llvm::Value *> mapping = {};

  println("MUTATE");
  println("  ", alloc);
  println("  ", type);

  // Get the original type.
  auto &orig_type = alloc.getType();

  // Find all the paths that needs to be updated in the type.
  auto differences = type_differences(orig_type, type);

  // MemOIRBuilder builder(alloc);

  // Change the type operand of the allocation.
  // auto *type_inst = builder.CreateTypeInst(type);
  // alloc.getTypeOperandAsUse().set(&type_inst->getCallInst());

  return mapping;
}

} // namespace llvm::memoir
