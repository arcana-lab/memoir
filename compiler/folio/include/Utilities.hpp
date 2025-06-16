#ifndef FOLIO_TRANSFORMS_UTILITIES_h
#define FOLIO_TRANSFORMS_UTILITIES_h

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/support/DataTypes.hpp"

namespace folio {

// Helper types.
template <typename T>
using LocalMap = llvm::memoir::SmallMap<llvm::Value *, T, /* SmallSize = */ 2>;

template <typename T>
using BaseMap = LocalMap<llvm::memoir::Set<T>>;

// Helper functions.
llvm::Function *parent_function(llvm::Value &V);

uint32_t forward_analysis(
    llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>>
        &encoded);

bool is_last_index(llvm::Use *use,
                   llvm::memoir::AccessInst::index_op_iterator index_end);

llvm::GlobalVariable &create_global_ptr(llvm::Module &module,
                                        const llvm::Twine &name = "",
                                        bool is_external = false);

llvm::AllocaInst &create_stack_ptr(llvm::Function &func,
                                   const llvm::Twine &name = "");

} // namespace folio

#endif // FOLIO_TRANSFORMS_UTILITIES_h
