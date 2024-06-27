#ifndef MEMOIR_TRANSFORMS_NORMALIZATION_TYPEINFERENCE_H
#define MEMOIR_TRANSFORMS_NORMALIZATION_TYPEINFERENCE_H

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/FunctionType.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"

namespace llvm::memoir {

// Inferred type
using inferred_type = tuple<bool, Type *>;

/**
 * Type inference for a MEMOIR program.
 */
class TypeInference {
public:
  /**
   * Construct a new TypeInference object for a given LLVM module.
   */
  TypeInference(llvm::Module &M) : M(M) {}

  /**
   * Perform type inference, analyzing the LLVM module first, then inserting
   * explicit type annotations.
   */
  bool run();

protected:
  /**
   * Run type inference on the module, annotating all untyped MEMOIR variables.
   */
  bool infer(llvm::Module &M);

  /**
   * Run type inference on the function, annotating all untyped MEMOIR
   * variables.
   */
  bool infer(llvm::Function &F);

  /**
   * Tries to infer the type of the given argument.
   */
  bool infer_argument_type(llvm::Argument &A);

  /**
   * Tries to infer the return type of the given function.
   */
  bool infer_return_type(llvm::Function &F);

  /**
   * Annotates the given module with type information.
   */
  bool annotate(llvm::Module &M);

  /**
   * Annotates the given argument with the specified type.
   */
  void annotate_argument_type(llvm::Argument &A, Type &type);

  /**
   * Annotates the given function with the specified return type.
   */
  void annotate_return_type(llvm::Function &F, Type &type);

  // Owned state.
  set<llvm::Function *> visited; // CURRENTLY UNUSED
  set<llvm::Function *> typed;   // CURRENTLY UNUSED

  // Borrowed state.
  llvm::Module &M;
  map<llvm::Argument *, memoir::Type *> argument_types_to_annotate;
  map<llvm::Function *, memoir::Type *> return_types_to_annotate;
};

} // namespace llvm::memoir

#endif
