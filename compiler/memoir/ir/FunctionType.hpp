#ifndef MEMOIR_IR_FUNCTIONTYPE_H
#define MEMOIR_IR_FUNCTIONTYPE_H

#include <variant>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include "memoir/ir/Types.hpp"

#include "memoir/support/DataTypes.hpp"

namespace memoir {

struct Type;

struct FunctionType {
public:
  static FunctionType &get(llvm::FunctionType &FT,
                           Type *return_type,
                           OrderedMap<unsigned, Type *> param_types);

  llvm::FunctionType &getLLVMFunctionType() const;
  std::variant<Type *, llvm::Type *> getReturnType() const;
  unsigned getNumParams() const;
  std::variant<Type *, llvm::Type *> getParamType(unsigned param_index) const;

protected:
  // Owned state

  // Borrowed state
  llvm::FunctionType &FT;
  Type *return_type; // if NULL, then it is an LLVM type.
  OrderedMap<unsigned, Type *> param_types;

  FunctionType(llvm::FunctionType &FT,
               Type *return_type,
               OrderedMap<unsigned, Type *> param_types)
    : FT(FT),
      return_type(return_type),
      param_types(param_types) {}
  ~FunctionType() {}
};

} // namespace memoir

#endif // MEMOIR_IR_FUNCTIONTYPE_H
