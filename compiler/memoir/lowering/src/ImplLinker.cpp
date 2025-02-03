#include <sstream>

#include "memoir/lowering/ImplLinker.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

namespace detail {
void register_default_implementations() {
  Implementation::define({
#if ENABLE_MULTIMAP
      Implementation( // std::multimap<T, U>
          ASSOC_SEQ_IMPL,
          AssocType::get(TypeVariable::get(),
                         SequenceType::get(TypeVariable::get()))),
#endif

      Implementation( // std::vector<T>
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

ImplLinker::ImplLinker(llvm::Module &M) : M(M), TC(M.getContext()) {
  // Register the default implementations.
  detail::register_default_implementations();
}

const Implementation &ImplLinker::get_default_implementation(
    CollectionType &type) {

  if (auto *seq_type = dyn_cast<SequenceType>(&type)) {
    return MEMOIR_SANITIZE(
        Implementation::lookup(SEQ_IMPL),
        "Failed to find the default implementation (" SEQ_IMPL ")");
  } else if (auto *assoc_type = dyn_cast<AssocType>(&type)) {
    auto &element_type = assoc_type->getElementType();
    if (isa<VoidType>(&element_type)) {
      return MEMOIR_SANITIZE(
          Implementation::lookup(SET_IMPL),
          "Failed to find the default implementation (" SET_IMPL ")");
    }
#if ENABLE_MULTIMAP
    else if (isa<SequenceType>(&element_type)) {
      return MEMOIR_SANITIZE(
          Implementation::lookup(ASSOC_SEQ_IMPL),
          "Failed to find the default implementation (" ASSOC_SEQ_IMPL ")");
    }
#endif
    else {
      return MEMOIR_SANITIZE(
          Implementation::lookup(ASSOC_IMPL),
          "Failed to find the default implementation (" ASSOC_IMPL ")");
    }
  }

  warnln(type);
  MEMOIR_UNREACHABLE("Collection type has no default implementation!");
}

void ImplLinker::implement(Type &type) {
  if (auto *struct_type = dyn_cast<StructType>(&type)) {
    this->structs_to_emit.insert(struct_type);
  }
}

void ImplLinker::implement(Instantiation &inst) {

  auto &types = inst.types();

  for (auto *type : types) {
    this->implement(*type);
  }

  // Ensure that this instantiation is unique.
  auto it = this->collections_to_emit.begin();
  for (; it != this->collections_to_emit.end(); ++it) {
    auto &it_inst = **it;
    if (it_inst.get_name() != inst.get_name()) {
      continue;
    }

    auto &it_types = it_inst.types();
    auto types_are_equal =
        std::equal(types.begin(),
                   types.end(),
                   it_types.begin(),
                   it_types.end(),
                   [&](const auto *lhs, const auto *rhs) {
                     return lhs->get_code() == rhs->get_code();
                   });
    if (types_are_equal) {
      return;
    }
  }

  this->collections_to_emit.insert(&inst);
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
    return "char *";
  } else if (isa<VoidType>(&T)) {
    return "void";
  } else if (auto *ref_type = dyn_cast<ReferenceType>(&T)) {
    return memoir_to_c_type(ref_type->getReferencedType()) + " *";
  } else if (auto *struct_type = dyn_cast<StructType>(&T)) {
    return "impl__" + struct_type->getName();
  } else if (auto *collection_type = dyn_cast<CollectionType>(&T)) {
    return "char *"; // TODO: attach attributes here: restrict, etc
  }

  MEMOIR_UNREACHABLE("Attempting to create Impl for unknown type!");
}

void ImplLinker::emit(llvm::raw_ostream &os) {
  // General include headers.
  fprintln(os, "#include <stdint.h>");
  fprintln(os, "#include <array>");
  fprintln(os, "#include <backend/utilities.h>");

  // Instantiate the struct implementations.
  for (auto *struct_type : this->structs_to_emit) {
    // Convert the struct to a type layout.
    auto &struct_layout = TC.convert(*struct_type);

    // Get the size of the struct layout in bytes.
    auto &data_layout = this->M.getDataLayout();
    auto struct_size =
        data_layout.getTypeAllocSize(&struct_layout.get_llvm_type());

    // Create a C struct for it.
    auto type_name = memoir_to_c_type(struct_layout.get_memoir_type());
    fprintln(os,
             "struct ",
             type_name,
             " : public Bytes<",
             struct_size,
             "> {};");
    fprintln(os, "DEF_HASH(", type_name, ");");
  }

  // Instantiate all of the collection implementations.
  for (auto *instantiation : this->collections_to_emit) {
    auto name = instantiation->get_name();

    fprintln(os, "");

    auto type_id = 0;
    for (auto *type : instantiation->types()) {

      // Emit the type code.
      fprintln(os,
               "#define CODE_",
               std::to_string(type_id),
               " ",
               type->get_code().value());

      // Convert the type.
      auto &layout = TC.convert(*type);

      // Emit the C type.
      fprintln(os,
               "#define TYPE_",
               std::to_string(type_id),
               " ",
               memoir_to_c_type(*type));

      ++type_id;
    }

    // Instantiate the collection.
    fprintln(os,
             "#include <backend/",
             instantiation->get_name(),
             "/instantiation.h>");

    // Undef
    type_id = 0;
    for (auto *type : instantiation->types()) {

      // Emit the type code.
      fprintln(os, "#undef CODE_", std::to_string(type_id));

      // Convert the type.
      auto &layout = TC.convert(*type);

      // Emit the C type.
      fprintln(os, "#undef TYPE_", std::to_string(type_id));

      ++type_id;
    }
  }
}

} // namespace llvm::memoir
