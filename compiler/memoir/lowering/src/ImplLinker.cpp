#include <sstream>

#include "memoir/lowering/ImplLinker.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

std::string default_seq_impl;
static llvm::cl::opt<std::string, true> DefaultSeqImpl(
    "default-seq-impl",
    llvm::cl::desc("Default implementation of Seq<T> collections."),
    llvm::cl::value_desc("typename"),
    llvm::cl::location(default_seq_impl),
    llvm::cl::init("stl_vector"));

std::string default_map_impl;
static llvm::cl::opt<std::string, true> DefaultMapImpl(
    "default-map-impl",
    llvm::cl::desc("Default implementation of Assoc<K,V> collections."),
    llvm::cl::value_desc("typename"),
    llvm::cl::location(default_map_impl),
    llvm::cl::init("stl_unordered_map"));

std::string default_set_impl;
static llvm::cl::opt<std::string, true> DefaultSetImpl(
    "default-set-impl",
    llvm::cl::desc("Default implementation of Assoc<K, void> collections."),
    llvm::cl::value_desc("typename"),
    llvm::cl::location(default_set_impl),
    llvm::cl::init("stl_unordered_set"));

namespace detail {
void register_default_implementations() {
  Implementation::define(
      { Implementation(default_seq_impl,
                       SequenceType::get(TypeVariable::get()),
                       /* selectable? */ true,
                       /* default? */ true),

        Implementation(default_map_impl,
                       AssocType::get(TypeVariable::get(), TypeVariable::get()),
                       /* selectable? */ true,
                       /* default? */ true),

        Implementation(default_set_impl,
                       AssocType::get(TypeVariable::get(), VoidType::get()),
                       /* selectable? */ true,
                       /* default? */ true),

        Implementation( // std::vector<T>
            "stl_vector",
            SequenceType::get(TypeVariable::get())),

        Implementation( // std::unordered_map<T, U>
            "stl_unordered_map",
            AssocType::get(TypeVariable::get(), TypeVariable::get())),

        Implementation( // std::unordered_set<T>
            "stl_unordered_set",
            AssocType::get(TypeVariable::get(), VoidType::get())),

#ifdef BOOST_INCLUDE_DIR
        Implementation( // boost::flat_Set<T>
            "boost_flat_set",
            AssocType::get(TypeVariable::get(), VoidType::get())),
        Implementation( // boost::flat_map<T>
            "boost_flat_map",
            AssocType::get(TypeVariable::get(), TypeVariable::get())),
#endif

        Implementation( // absl::flat_hash_set<T>
            "abseil_flat_hash_set",
            AssocType::get(TypeVariable::get(), VoidType::get())),
        Implementation( // boost::flat_hash_map<T>
            "abseil_flat_hash_map",
            AssocType::get(TypeVariable::get(), TypeVariable::get())),

        Implementation("bitset",
                       AssocType::get(TypeVariable::get(), VoidType::get()),
                       /* selectable? */ false),
        Implementation("bitmap",
                       AssocType::get(TypeVariable::get(), TypeVariable::get()),
                       /* selectable? */ false),
        Implementation("sparse_bitset",
                       AssocType::get(TypeVariable::get(), VoidType::get()),
                       /* selectable? */ false),
        Implementation("sparse_bitmap",
                       AssocType::get(TypeVariable::get(), TypeVariable::get()),
                       /* selectable? */ false),
        Implementation("twined_bitmap",
                       AssocType::get(TypeVariable::get(), TypeVariable::get()),
                       /* selectable? */ false) });
}
} // namespace detail

ImplLinker::ImplLinker(llvm::Module &M) : M(M), TC(M.getContext()) {
  // Register the default implementations.
  detail::register_default_implementations();
}

const Implementation &ImplLinker::get_default_implementation(
    CollectionType &type) {

  if (auto *seq_type = dyn_cast<SequenceType>(&type)) {
    return MEMOIR_SANITIZE(Implementation::lookup(default_seq_impl),
                           "Failed to find the default implementation (",
                           default_seq_impl,
                           ")");
  } else if (auto *assoc_type = dyn_cast<AssocType>(&type)) {
    auto &element_type = assoc_type->getElementType();
    if (isa<VoidType>(&element_type)) {
      return MEMOIR_SANITIZE(Implementation::lookup(default_set_impl),
                             "Failed to find the default implementation (",
                             default_set_impl,
                             ")");
    } else {
      return MEMOIR_SANITIZE(Implementation::lookup(default_map_impl),
                             "Failed to find the default implementation (",
                             default_map_impl,
                             ")");
    }
  }

  warnln(type);
  MEMOIR_UNREACHABLE("Collection type has no default implementation!");
}

const Implementation &ImplLinker::get_implementation(CollectionType &type) {

  // Fetch the selection.
  auto selection = type.get_selection();

  // If there is not selection, or the default is requested, fetch it.
  if (not selection.has_value() or selection.value() == ":DEFAULT:") {
    return ImplLinker::get_default_implementation(type);
  }

  // Otherwise, lookup the implementation.
  return MEMOIR_SANITIZE(Implementation::lookup(selection.value()),
                         "Requested implementation (",
                         selection.value(),
                         ") has not been registered with the compiler!");
}

Type &ImplLinker::canonicalize(Type &type) {

  // Recurse based on the next offset, rebuilding the type with the returnee.
  if (auto *collection_type = dyn_cast<CollectionType>(&type)) {

    // If the selection is a placeholder, replace it with the canon selection.
    auto selection = collection_type->get_selection();
    if (not selection or selection.value() == ":DEFAULT:") {
      // Get the default implementation of this type.
      const auto &default_impl =
          ImplLinker::get_default_implementation(*collection_type);

      // Update the selection.
      selection = default_impl.get_name();
    }

    // Recurse on the element type.
    auto &elem_type = this->canonicalize(collection_type->getElementType());

    // Rebuild the type.
    if (auto *assoc_type = dyn_cast<AssocType>(collection_type)) {
      return AssocType::get(assoc_type->getKeyType(), elem_type, selection);

    } else if (auto *seq_type = dyn_cast<SequenceType>(collection_type)) {
      return SequenceType::get(elem_type, selection);

    } else if (auto *array_type = dyn_cast<ArrayType>(collection_type)) {
      return ArrayType::get(elem_type, array_type->getLength());
    }

  } else if (auto *tuple_type = dyn_cast<TupleType>(&type)) {

    auto num_fields = tuple_type->getNumFields();
    Vector<Type *> fields(num_fields, NULL);
    for (unsigned field = 0; field < num_fields; ++field) {
      fields[field] = &this->canonicalize(tuple_type->getFieldType(field));
    }

    return TupleType::get(fields);
  }

  return type;
}

void ImplLinker::implement(Type &type) {

  if (auto *tuple_type = dyn_cast<TupleType>(&type)) {

    // Skip types we've already implemented.
    for (const auto &to_emit : this->structs_to_emit) {
      if (&to_emit.type() == tuple_type) {
        return;
      }
    }

    // Instantitiate the fields of this struct.
    Vector<Instantiation *> fields = {};
    for (auto field = 0; field < tuple_type->getNumFields(); ++field) {
      auto &field_type = tuple_type->getFieldType(field);

      if (auto *field_collection_type = dyn_cast<CollectionType>(&field_type)) {

        const auto &impl = this->get_implementation(*field_collection_type);

        auto &inst = impl.instantiate(*field_collection_type);

        fields.push_back(&inst);
        continue;
      }

      fields.push_back(nullptr);
    }

    this->structs_to_emit.emplace(tuple_type, fields);
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

static std::string memoir_to_c_type(Type &T) {

  if (auto *integer_type = dyn_cast<IntegerType>(&T)) {
    std::stringstream ss;

    if (integer_type->getBitWidth() == 1) {
      return "bool";
    }

    if (!integer_type->isSigned()) {
      ss << "u";
    };

    auto bitwidth = get_next_size(integer_type->getBitWidth());
    ss << "int" << std::to_string(bitwidth) << "_t";

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
  } else if (auto *tuple_type = dyn_cast<TupleType>(&T)) {
    return tuple_type->get_code().value();
  } else if (auto *collection_type = dyn_cast<CollectionType>(&T)) {
    return "char *"; // TODO: attach attributes here: restrict, etc
  }

  MEMOIR_UNREACHABLE("Attempting to create Impl for unknown type!");
}

static void include_collection_file(llvm::raw_ostream &os,
                                    const Instantiation &instantiation,
                                    std::string filename) {

  auto type_id = 0;
  for (auto *type : instantiation.types()) {

    // Emit the type code.
    fprintln(os,
             "#define CODE_",
             std::to_string(type_id),
             " ",
             type->get_code().value());

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
           instantiation.get_name(),
           "/",
           filename,
           ">");

  // Undef
  type_id = 0;
  for (auto *type : instantiation.types()) {

    // Emit the type code.
    fprintln(os, "#undef CODE_", std::to_string(type_id));

    // Emit the C type.
    fprintln(os, "#undef TYPE_", std::to_string(type_id));

    ++type_id;
  }

  fprintln(os);

  return;
}

static void define_tuple(llvm::raw_ostream &os,
                         const StructInstantiation &inst) {

  auto &type = inst.type();
  const auto &fields = inst.fields();

  // Create a C struct for it.
  auto type_name = type.get_code().value();

  fprintln(os, "#pragma pack(1)");
  fprintln(os, "struct alignas(8)", type_name, " { ");
  for (auto field = 0; field < type.getNumFields(); ++field) {
    auto *field_inst = fields[field];
    if (field_inst) {
      fprintln(os,
               "  ",
               field_inst->get_typename(),
               " f_",
               std::to_string(field),
               ";");
    } else {
      auto &field_type = type.getFieldType(field);

      fprint(os, "  ");

      // Print the type.
      fprint(os, memoir_to_c_type(field_type));

      // Print the field name.
      fprint(os, " f_", std::to_string(field));

      // If this is a bitfield, print its width.
      if (auto *field_int_type = dyn_cast<IntegerType>(&field_type)) {
        auto bitwidth = field_int_type->getBitWidth();
        fprint(os, " : ", bitwidth);
      }

      fprintln(os, ";");
    }
  }

  fprintln(os, " };");
}

static void emit_collection(llvm::raw_ostream &os,
                            const Instantiation &inst,
                            Set<const Instantiation *> &collections_declared,
                            Set<const StructInstantiation *> &tuples_declared) {

  // If the collection has already been declared, skip.
  if (collections_declared.count(&inst)) {
    return;
  }

  // Define the collection.
  include_collection_file(os, inst, "declaration.h");

  // Mark the collection as emitted.
  collections_declared.insert(&inst);

  return;
}

static void emit_tuple(llvm::raw_ostream &os,
                       const StructInstantiation &inst,
                       Set<const Instantiation *> &collections_declared,
                       Set<const StructInstantiation *> &tuples_declared) {

  if (tuples_declared.count(&inst)) {
    return;
  }

  // Instantiate fields.
  for (auto *field_inst : inst.fields()) {
    // Skip non-collection fields.
    if (not field_inst) {
      continue;
    }

    // If the collection has already been emitted, skip it.
    if (collections_declared.count(field_inst)) {
      continue;
    }

    emit_collection(os, *field_inst, collections_declared, tuples_declared);
  }

  // Define the tuple.
  define_tuple(os, inst);
  tuples_declared.insert(&inst);
}

void ImplLinker::emit(llvm::raw_ostream &os) {
  // General include headers.
  fprintln(os, "#include <stdint.h>");
  fprintln(os, "#include <array>");
  fprintln(os, "#include <backend/utilities.h>");

  // Track the instantiations that have been used already.
  Set<const Instantiation *> collections_declared = {};
  Set<const StructInstantiation *> tuples_declared = {};

  // Create forward declarations of all struct types.
  fprintln(os);
  fprintln(os);
  fprintln(os, "// Forward declarations for struct types");
  for (const auto &tuple : this->structs_to_emit) {
    emit_tuple(os, tuple, collections_declared, tuples_declared);
  }

  // Create forward declarations of all collection implementations.
  fprintln(os);
  fprintln(os);
  fprintln(os, "// Forward declarations for collection types");
  for (const auto *inst : this->collections_to_emit) {
    emit_collection(os, *inst, collections_declared, tuples_declared);
  }

  // // Emit the struct access functions.
  // fprintln(os);
  // fprintln(os);
  // fprintln(os, "// Definition of struct types.");
  // for (const auto &[tuple_type, fields] : this->structs_to_emit) {
  //   if (defined.count(tuple_type)) {
  //     continue;
  //   }
  //   define_tuple(os, *tuple_type, fields);
  // }

  // Instantiate the collections.
  fprintln(os);
  fprintln(os);
  fprintln(os, "// Collection access functions.");
  for (const auto *instantiation : this->collections_to_emit) {
    include_collection_file(os, *instantiation, "instantiation.h");
  }

  // Emit the struct access functions.
  fprintln(os);
  fprintln(os);
  fprintln(os, "// Struct access functions.");
  for (const auto &[tuple_type, fields] : this->structs_to_emit) {

    // Create a C struct for it.
    auto type_name = tuple_type->get_code().value();

    // Emit the allocation function for this struct.
    fprintln(os,
             "CNAME ALWAYS_INLINE USED ",
             type_name,
             "* ",
             type_name,
             "__allocate(void)",
             " { return new ",
             type_name,
             "(); }");

    // Create functions for each operation.
    for (auto field = 0; field < tuple_type->getNumFields(); ++field) {
      auto &field_type = tuple_type->getFieldType(field);
      auto c_field_type = memoir_to_c_type(field_type);

      auto *field_inst = fields[field];

      if (isa<ObjectType>(&field_type)) {
        fprintln(os,
                 "CNAME ALWAYS_INLINE USED ",
                 "char * ",
                 type_name,
                 "__get_",
                 field,
                 "(",
                 type_name,
                 "* x",
                 ") { return (char *)&x->f_",
                 field,
                 "; }");
      } else {
        fprintln(os,
                 "CNAME ALWAYS_INLINE USED ",
                 c_field_type,
                 " ",
                 type_name,
                 "__read_",
                 std::to_string(field),
                 "(",
                 type_name,
                 "* x",
                 ") { return x->f_",
                 std::to_string(field),
                 "; }");
        fprintln(os,
                 "CNAME ALWAYS_INLINE USED ",
                 "void ",
                 type_name,
                 "__write_",
                 field,
                 "(",
                 type_name,
                 "* obj",
                 ", ",
                 c_field_type,
                 " val",
                 ") { obj->f_",
                 field,
                 " = val; }");
      }
    }

    fprintln(os);
  }
}

} // namespace llvm::memoir
