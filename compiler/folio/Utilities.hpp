#ifndef FOLIO_TRANSFORMS_UTILITIES_h
#define FOLIO_TRANSFORMS_UTILITIES_h

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

#include "memoir/ir/ControlFlow.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/UnionFind.hpp"

namespace folio {

// Using memoir types.
using llvm::memoir::List;
using llvm::memoir::Map;
using llvm::memoir::Option;
using llvm::memoir::OrderedSet;
using llvm::memoir::Pair;
using llvm::memoir::Set;
using llvm::memoir::SmallSet;
using llvm::memoir::UnionFind;
using llvm::memoir::Vector;

// Helper types.
template <typename T>
using LocalMap = llvm::memoir::SmallMap<llvm::Value *, T, /* SmallSize = */ 2>;

template <typename T>
using BaseMap = LocalMap<llvm::memoir::Set<T>>;

// Helper functions.
llvm::Function *parent_function(llvm::Value &V);
llvm::Function *parent_function(llvm::Use &U);

void erase_uses(Set<llvm::Use *> &uses, const Set<llvm::Use *> &to_erase);
template <typename Key, typename Uses>
void erase_uses(Map<Key, Uses> &uses, const Set<llvm::Use *> &to_erase) {
  for (auto &[_, inner] : uses)
    erase_uses(inner, to_erase);
}

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

llvm::Instruction *insertion_point(llvm::Use &use);

// Debug
void print_uses(const Vector<llvm::Use *> &uses);
void print_uses(const Set<llvm::Use *> &uses);
template <typename Key, typename Uses>
void print_uses(const Map<Key, Uses> &uses) {
  for (const auto &[_, inner] : uses)
    print_uses(inner);
}

} // namespace folio

#endif // FOLIO_TRANSFORMS_UTILITIES_h
