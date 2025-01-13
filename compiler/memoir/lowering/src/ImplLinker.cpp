#include <sstream>

#include "memoir/lowering/ImplLinker.hpp"

#include "memoir/utility/Metadata.hpp"

// Default implementations
#define ASSOC_IMPL "stl_unordered_map"
#define SET_IMPL "stl_unordered_set"
#define SEQ_IMPL "stl_vector"

namespace llvm::memoir {

namespace detail {
void register_default_implementations() {
  Implementation::define(
      { Implementation( // std::vector<T>
            SEQ_IMPL,
            SequenceType::get(TypeVariable::get())),

        Implementation( // std::unordered_map<T, U>
            ASSOC_IMPL,
            AssocType::get(TypeVariable::get(), TypeVariable::get())),

        Implementation( // std::unordered_set<T>
            SET_IMPL,
            AssocType::get(TypeVariable::get(), VoidType::get()))

      });
}
} // namespace detail

ImplLinker::ImplLinker(llvm::Module &M) : M(M) {
  // Register the default implementations.
  detail::register_default_implementations();
}

const Implementation &ImplLinker::get_default_implementation(
    CollectionType &type) {

  if (auto *seq_type = dyn_cast<SequenceType>(&type)) {
    return MEMOIR_SANITIZE(
        Implementation::lookup(SEQ_IMPL),
        "Failed to find the default sequence implementation");
  } else if (auto *assoc_type = dyn_cast<AssocType>(&type)) {
    if (isa<VoidType>(assoc_type->getElementType())) {
      return MEMOIR_SANITIZE(Implementation::lookup(SET_IMPL),
                             "Failed to find the default set implementation");
    } else {
      return MEMOIR_SANITIZE(Implementation::lookup(ASSOC_IMPL),
                             "Failed to find the default assoc implementation");
    }
  }

  warnln(type);
  MEMOIR_UNREACHABLE("Collection type has no default implementation!");
}

void ImplLinker::implement_type(TypeLayout &type_layout) {
  // Unpack the type layout.
  auto &llvm_type = type_layout.get_llvm_type();

  // If it is a struct type, add it to set of structs to implement.
  if (isa<llvm::StructType>(&llvm_type)) {
    this->struct_implementations.insert(&type_layout);
  }

  return;
}

void ImplLinker::implement_seq(std::string impl_name,
                               TypeLayout &element_layout) {

  this->implement_type(element_layout);

  if (isa<CollectionType>(&element_layout.get_memoir_type())) {
    auto range = this->seq_implementations.equal_range(impl_name);
    for (auto it = range.first; it != range.second; ++it) {
      auto *layout = it->second;
      if (isa<CollectionType>(layout->get_memoir_type())) {
        return;
      }
    }
  }

  insert_unique(this->seq_implementations, impl_name, &element_layout);

  return;
}

void ImplLinker::implement_assoc(std::string impl_name,
                                 TypeLayout &key_layout,
                                 TypeLayout &value_layout) {
  this->implement_type(key_layout);
  this->implement_type(value_layout);

  if (isa<CollectionType>(&value_layout.get_memoir_type())) {
    auto range = this->assoc_implementations.equal_range(impl_name);
    for (auto it = range.first; it != range.second; ++it) {
      auto [_, layout] = it->second;
      if (isa<CollectionType>(layout->get_memoir_type())) {
        return;
      }
    }
  }

  insert_unique(this->assoc_implementations,
                impl_name,
                std::make_tuple(&key_layout, &value_layout));

  // For the time being, we will need to instantiate the stl_vector for the key
  // type to handle keys. Properly handling the keys iterator as a collection
  // all its own is future work.
  if (impl_name == "stl_unordered_map" || impl_name == "stl_map"
      || impl_name == "stl_unordered_set") {
    this->implement_seq("stl_vector", key_layout);
  }

  return;
}

// Utility functions.
static std::string memoir_to_c_type(Type &T) {

  if (auto *integer_type = dyn_cast<IntegerType>(&T)) {
    std::stringstream ss;

    if (integer_type->getBitWidth() == 1) {
      return "bool";
    }

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
    return "void *";
  } else if (isa<VoidType>(&T)) {
    return "void";
  } else if (auto *ref_type = dyn_cast<ReferenceType>(&T)) {
    return memoir_to_c_type(ref_type->getReferencedType()) + " *";
  } else if (auto *struct_type = dyn_cast<StructType>(&T)) {
    return "impl__" + struct_type->getName();
  } else if (auto *collection_type = dyn_cast<CollectionType>(&T)) {
    return "void *"; // TODO: attach attributes here: restrict, etc
  }

  MEMOIR_UNREACHABLE("Attempting to create Impl for unknown type!");
}

void ImplLinker::emit(llvm::raw_ostream &os) {
  // General include headers.
  fprintln(os, "#include <stdint.h>");
  fprintln(os, "#include <array>");

  // Instantiate the struct implementations.
  for (auto *struct_layout : this->struct_implementations) {
    // Get the size of the struct layout in bytes.
    auto &data_layout = this->M.getDataLayout();
    auto struct_size =
        data_layout.getTypeAllocSize(&struct_layout->get_llvm_type());

    // Create a C struct for it.
    auto type_name = memoir_to_c_type(struct_layout->get_memoir_type());
    fprintln(os,
             "typedef struct _",
             type_name,
             " {\n",
             "  std::array<uint8_t, ",
             struct_size,
             "> _storage;\n",
             "}",
             type_name,
             ";");
  }

  // Instantiate the sequence implementations.
  for (auto it = this->seq_implementations.begin();
       it != this->seq_implementations.end();) {

    auto impl_name = it->first;

    fprintln(os, "#include \"backend/", impl_name, ".h\"");

    for (; it != this->seq_implementations.upper_bound(impl_name); ++it) {
      auto *elem = it->second;
      auto &elem_type = elem->get_memoir_type();
      auto elem_code = *elem_type.get_code();
      auto c_type = memoir_to_c_type(elem_type);

      if (c_type == "bool") {
        fprintln(os,
                 "INSTANTIATE_NO_REF_",
                 impl_name,
                 "(",
                 elem_code,
                 ", ",
                 c_type,
                 ")");
      } else if (isa<StructType>(&elem_type) or isa<ArrayType>(&elem_type)) {
        fprintln(os,
                 "INSTANTIATE_NESTED_",
                 impl_name,
                 "(",
                 elem_code,
                 ", ",
                 c_type,
                 ")");
      } else {
        fprintln(os,
                 "INSTANTIATE_",
                 impl_name,
                 "(",
                 elem_code,
                 ", ",
                 c_type,
                 ")");
      }
    }
  }

  // Instantiate the assoc implementations.
  for (auto it = this->assoc_implementations.begin();
       it != this->assoc_implementations.end();) {

    auto impl_name = it->first;

    fprintln(os, "#include \"backend/", impl_name, ".h\"");

    for (; it != this->assoc_implementations.upper_bound(impl_name); ++it) {
      auto [key, value] = it->second;
      auto &key_type = key->get_memoir_type();
      auto key_code = *key_type.get_code();
      auto c_key = memoir_to_c_type(key_type);

      auto &value_type = value->get_memoir_type();
      auto value_code = *value_type.get_code();
      auto c_value = memoir_to_c_type(value_type);

      fprintln(os,
               "INSTANTIATE_",
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
