#ifndef MEMOIR_IMPLLINKER_H
#define MEMOIR_IMPLLINKER_H

#include "memoir/support/InternalDatatypes.hpp"

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
  ImplLinker(llvm::Module &M) : M(M) {}
  ~ImplLinker() {}

  /**
   * Get the name of the implementation for the given instruction.
   *
   * @param I the llvm::Instruction
   * @param type the MEMOIR type of I
   * @returns the implementation name as a string
   */
  static std::string get_implementation_name(llvm::Instruction &I,
                                             CollectionType &type);

  /**
   * Get the implementation's function prefix for the given instruction.
   *
   * @param I the llvm::Instruction
   * @param type the MEMOIR type of I
   * @returns the implementation's function prefix as a string
   */
  static std::string get_implementation_prefix(llvm::Instruction &I,
                                               CollectionType &type);

  void implement_seq(std::string impl_name, TypeLayout &element_type_layout);

  void implement_assoc(std::string impl_name,
                       TypeLayout &key_type_layout,
                       TypeLayout &value_type_layout);

  void implement_type(TypeLayout &struct_type_layout);

  void emit(llvm::raw_ostream &os = llvm::errs());

protected:
  ordered_set<TypeLayout *> struct_implementations;
  ordered_multimap<std::string, TypeLayout *> seq_implementations;
  ordered_multimap<std::string, tuple<TypeLayout *, TypeLayout *>>
      assoc_implementations;

  llvm::Module &M;

}; // class ImplLinker

} // namespace llvm::memoir

#endif // MEMOIR_IMPLLINKER_H
