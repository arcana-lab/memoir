#ifndef MEMOIR_IMPLLINKER_H
#define MEMOIR_IMPLLINKER_H

#include "llvm/Support/CommandLine.h"

#include "memoir/support/DataTypes.hpp"

#include "memoir/lowering/Implementation.hpp"
#include "memoir/lowering/TypeLayout.hpp"

/*
 * This file provides a utility which instantiates the necessary collection
 * implementations, this information is consumed by the linker to generate
 * the collections which store user-defined objects.
 *
 * Author(s): Tommy McMichen
 * Created: September 27, 2023
 */

namespace memoir {

// Default implementations
extern std::string default_seq_impl;
extern std::string default_map_impl;
extern std::string default_set_impl;

struct StructInstantiation {
  TupleType *_type;
  Vector<Instantiation *> _fields;

  TupleType &type() const {
    return *this->_type;
  }

  Vector<Instantiation *> &fields() {
    return this->_fields;
  }

  const Vector<Instantiation *> &fields() const {
    return this->_fields;
  }

  StructInstantiation(TupleType *type, llvm::ArrayRef<Instantiation *> fields)
    : _type(type),
      _fields(fields.begin(), fields.end()) {}

  bool operator<(const StructInstantiation &other) const {
    return _type < other._type;
  }
};

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
  static const Implementation &get_implementation(CollectionType &type);

  /**
   * Canonicalize any default selections in the given type, replacing them with
   * the default implementation.
   */
  Type &canonicalize(Type &type);

  void implement(Type &type);

  void implement(Instantiation &inst);

  void emit(llvm::raw_ostream &os = llvm::errs());

protected:
  OrderedSet<StructInstantiation> structs_to_emit;
  OrderedSet<Instantiation *> collections_to_emit;

  llvm::Module &M;
  TypeConverter TC;

}; // class ImplLinker

} // namespace memoir

#endif // MEMOIR_IMPLLINKER_H
