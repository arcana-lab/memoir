#ifndef MEMOIR_IMPLLINKER_H
#define MEMOIR_IMPLLINKER_H

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/lowering/Implementation.hpp"
#include "memoir/lowering/TypeLayout.hpp"

// Default implementations
#define ASSOC_IMPL "stl_unordered_map"
#ifdef BOOST_INCLUDE_DIR
#  define SET_IMPL "boost_flat_set"
#else
#  define SET_IMPL "stl_unordered_set"
#endif
#define SEQ_IMPL "stl_vector"
#define ASSOC_SEQ_IMPL "boost_flat_multimap"

#define ENABLE_MULTIMAP 0

/*
 * This file provides a utility which instantiates the necessary collection
 * implementations, this information is consumed by the linker to generate
 * the collections which store user-defined objects.
 *
 * Author(s): Tommy McMichen
 * Created: September 27, 2023
 */

namespace llvm::memoir {

class ImplLinker {
public:
  /**
   * Construct a new ImplLinker for the given LLVM module.
   */
  ImplLinker(llvm::Module &M);
  ~ImplLinker() {}

  /**
   * Get the default implementation for the given type.
   *
   * @param type of the collection
   * @returns the default implementation
   */
  static const Implementation &get_default_implementation(CollectionType &type);

  /**
   * Get the implementation for the given selection (default if not provided)
   * and type.
   *
   * @param optional selection name
   * @returns the selected implementation, or the default implementation if no
   * name was provided.
   */
  static const Implementation &get_implementation(
      const std::optional<std::string> &selection,
      CollectionType &type);

  void implement(Type &type);

  void implement(Instantiation &inst);

  void emit(llvm::raw_ostream &os = llvm::errs());

protected:
  struct StructInstantiation {
    StructType *type;
    vector<Instantiation *> fields;

    bool operator<(const StructInstantiation &other) const {
      return type < other.type;
    }
  };

  ordered_set<StructInstantiation> structs_to_emit;
  ordered_set<Instantiation *> collections_to_emit;

  llvm::Module &M;
  TypeConverter TC;

}; // class ImplLinker

} // namespace llvm::memoir

#endif // MEMOIR_IMPLLINKER_H
