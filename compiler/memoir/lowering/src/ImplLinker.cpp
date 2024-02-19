#include <sstream>

#include "memoir/lowering/ImplLinker.hpp"

namespace llvm::memoir {

void ImplLinker::implement_seq(std::string impl_name,
                               TypeLayout &element_type_layout) {

  insert_unique(this->seq_implementations, impl_name, &element_type_layout);
  return;
}

void ImplLinker::implement_assoc(std::string impl_name,
                                 TypeLayout &key_type_layout,
                                 TypeLayout &value_type_layout) {
  insert_unique(this->assoc_implementations,
                impl_name,
                std::make_tuple(&key_type_layout, &value_type_layout));
  return;
}

// Utility functions.
static std::string memoir_to_c_type(Type &T) {

  if (auto *integer_type = dyn_cast<IntegerType>(&T)) {
    std::stringstream ss;

    if (!integer_type->isSigned()) {
      ss << "u";
    };
    ss << "int" << std::to_string(integer_type->getBitWidth()) << "_t";

    return ss.str();
  } else if (isa<FloatType>(&T)) {
    return "float";
  } else if (isa<DoubleType>(&T)) {
    return "double";
  } else if (isa<PointerType>(&T)) {
    return "(void *)";
  } else if (auto *ref_type = dyn_cast<ReferenceType>(&T)) {
    return "(" + memoir_to_c_type(ref_type->getReferencedType()) + " *)";
  } else if (auto *struct_type = dyn_cast<StructType>(&T)) {
    return "impl__" + struct_type->getName();
  } else if (auto *collection_type = dyn_cast<CollectionType>(&T)) {
    MEMOIR_UNREACHABLE("Nested collections are not yet supported!");
  }

  MEMOIR_UNREACHABLE("Attempting to create Impl for unknown type!");
}

void ImplLinker::emit() {
  // Instantiate the sequence implementations.
  for (auto it = this->seq_implementations.begin();
       it != this->seq_implementations.end();) {

    auto impl_name = it->first;

    println("#include \"backend/", impl_name, ".h\"");

    for (; it != this->seq_implementations.upper_bound(impl_name); ++it) {
      auto *elem = it->second;
      auto &elem_type = elem->get_memoir_type();
      auto elem_code = *elem_type.get_code();
      auto c_type = memoir_to_c_type(elem_type);

      println("INSTANTIATE_", impl_name, "(", elem_code, ", ", c_type, ")");
    }
  }

  // Instantiate the assoc implementations.
  for (auto it = this->assoc_implementations.begin();
       it != this->assoc_implementations.end();) {

    auto impl_name = it->first;

    println("#include \"backend/", impl_name, ".h\"");

    for (; it != this->assoc_implementations.upper_bound(impl_name); ++it) {
      auto [key, value] = it->second;
      auto &key_type = key->get_memoir_type();
      auto key_code = *key_type.get_code();
      auto c_key = memoir_to_c_type(key_type);

      auto &value_type = value->get_memoir_type();
      auto value_code = *value_type.get_code();
      auto c_value = memoir_to_c_type(value_type);

      println("INSTANTIATE_",
              impl_name,
              "(",
              key_code,
              ", ",
              c_key,
              ", ",
              value_code,
              ", ",
              c_value,
              ")");
    }
  }
}

} // namespace llvm::memoir
