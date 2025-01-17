#ifndef MEMOIR_IMPLLINKER_H
#define MEMOIR_IMPLLINKER_H

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/lowering/Implementation.hpp"
#include "memoir/lowering/TypeLayout.hpp"

/*
 * This file provides a utility which instantiates the necessary collection
 * implementations, this information is consumed by the linker to generate the
 * collections which store user-defined objects.
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

  void implement(Type &type);

  void implement(Instantiation &inst);

  void emit(llvm::raw_ostream &os = llvm::errs());

protected:
  ordered_set<StructType *> structs_to_emit;
  ordered_set<Instantiation *> collections_to_emit;

  llvm::Module &M;
  TypeConverter TC;

}; // class ImplLinker

} // namespace llvm::memoir

#endif // MEMOIR_IMPLLINKER_H
