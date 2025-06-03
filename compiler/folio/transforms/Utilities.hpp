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

llvm::Function *parent_function(llvm::Value &V);

uint32_t forward_analysis(
    llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>>
        &encoded);

bool is_last_index(llvm::Use *use,
                   llvm::memoir::AccessInst::index_op_iterator index_end);

} // namespace folio

#endif // FOLIO_TRANSFORMS_UTILITIES_h
