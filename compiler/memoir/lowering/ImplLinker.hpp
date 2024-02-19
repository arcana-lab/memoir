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

  void implement_seq(std::string impl_name, TypeLayout &element_type_layout);

  void implement_assoc(std::string impl_name,
                       TypeLayout &key_type_layout,
                       TypeLayout &value_type_layout);

  void emit();

protected:
  ordered_multimap<std::string, TypeLayout *> seq_implementations;
  ordered_multimap<std::string, tuple<TypeLayout *, TypeLayout *>>
      assoc_implementations;

  llvm::Module &M;

}; // class ImplLinker

} // namespace llvm::memoir

#endif // MEMOIR_IMPLLINKER_H
