#ifndef MEMOIR_TRANSFORMS_DATAENUMERATION_UTILITIES_h
#define MEMOIR_TRANSFORMS_DATAENUMERATION_UTILITIES_h

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

#include "memoir/ir/ControlFlow.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/UnionFind.hpp"

namespace memoir {

// Helper types.
template <typename T>
using LocalMap = SmallMap<llvm::Value *, T, /* SmallSize = */ 2>;

template <typename T>
using BaseMap = LocalMap<Set<T>>;

// Helper functions.
llvm::Function *parent_function(llvm::Value &V);
llvm::Function *parent_function(llvm::Use &U);

void erase_uses(Set<llvm::Use *> &uses, const Set<llvm::Use *> &to_erase);
template <typename Key, typename Uses>
void erase_uses(Map<Key, Uses> &uses, const Set<llvm::Use *> &to_erase) {
  for (auto &[_, inner] : uses)
    erase_uses(inner, to_erase);
}

uint32_t forward_analysis(Map<llvm::Function *, Set<llvm::Value *>> &encoded);

bool is_last_index(llvm::Use *use, AccessInst::index_op_iterator index_end);

llvm::GlobalVariable &create_global_ptr(llvm::Module &module,
                                        const llvm::Twine &name = "",
                                        bool is_external = false);

llvm::AllocaInst &create_stack_ptr(llvm::Function &func,
                                   const llvm::Twine &name = "");

llvm::Instruction *insertion_point(llvm::Use &use);

// Debug
void print_uses(const Vector<llvm::Use *> &uses);
void print_uses(const Set<llvm::Use *> &uses);
template <typename Key, typename Uses>
void print_uses(const Map<Key, Uses> &uses) {
  for (const auto &[_, inner] : uses)
    print_uses(inner);
}

} // namespace memoir

#endif // MEMOIR_TRANSFORMS_DATAENUMERATION_UTILITIES_h
