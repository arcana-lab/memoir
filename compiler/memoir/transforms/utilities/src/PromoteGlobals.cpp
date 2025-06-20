#include <algorithm>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "memoir/ir/CallGraph.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/transforms/utilities/PromoteGlobals.hpp"
#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

bool global_is_promotable(llvm::GlobalVariable &global) {

  debugln("PROMOTABLE? ", global);

  // Check that the global is not externally visible.
  auto internal = global.hasInternalLinkage() or global.hasPrivateLinkage();
  if (not internal) {
    debugln("  NO, EXTERNAL!");
    return false;
  }

  // Check that the global has a parent module.
  if (not global.getParent()) {
    debugln("  NO, ORPHAN!");
    return false;
  }

  // Check that the global has a type.
  auto *type = global.getValueType();
  if (not type) {
    debugln("  NO, UNTYPED!");
    return false;
  }

  // Check that the global is a scalar type.
  if (type != type->getScalarType()) {
    debugln("  NO, NON-SCALAR!");
    return false;
  }

  // Check the the address of the global is never taken.
  for (auto &use : global.uses()) {
    auto *user = use.getUser();

    // Check if the use is as a pointer operand to a memory access.
    if (auto *store = dyn_cast<llvm::StoreInst>(user)) {
      auto &ptr_use = store->getOperandUse(1);
      if (&use == &ptr_use) {
        continue;
      }
    } else if (auto *load = dyn_cast<llvm::LoadInst>(user)) {
      auto &ptr_use = load->getOperandUse(0);
      if (&use == &ptr_use) {
        continue;
      }
    }

    // Otherwise, the address of the global may be taken.
    debugln("  NO, ADDR TAKEN ", *user);
    return false;
  }

  // If we got this far, the global is promotable!
  debugln("  YES!");
  return true;
}

struct FunctionPath : public Vector<llvm::Function *> {
  using Base = Vector<llvm::Function *>;

  /** Query if any internal functions in the path may be called externally. */
  bool called_externally(llvm::CallGraph &callgraph) const {
    auto *external = callgraph.getExternalCallingNode();

    Set<llvm::Function *> called_externally = {};
    for (const auto &[_, out] : *external) {
      if (auto *callee = out->getFunction()) {
        called_externally.insert(callee);
      }
    }

    // We only care about incoming edges along the path, so don't include the
    // first function.
    for (auto it = std::next(this->begin()); it != this->end(); ++it) {
      auto *func = *it;
      if (called_externally.contains(func)) {
        return true;
      }
    }

    return false;
  }

  /** Check if the path has been duplicated */
  bool duplicated() const {

    auto rit = this->rbegin(), rie = this->rend();
    if (rit == rie) {
      return false;
    }

    auto *func = *rit;

    // Find the last instance of this function in the path.
    auto found = std::find(std::next(rit), rie, func);

    // Check if the path ranges are the same.
    auto it = std::next(rit), ie = found;
    auto jt = std::next(found), je = rie;
    for (; it != ie and jt != je; ++it, ++jt) {
      if (*it != *jt) {
        return false;
      }
    }

    return true;
  }

  /** Check if this path is a subset of another. */
  bool subset_of(const FunctionPath &other) const {
    return std::search(other.begin(), other.end(), this->begin(), this->end())
           != other.end();
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const FunctionPath &path) {
    bool first = true;
    for (auto *func : path) {
      if (first) {
        first = false;
      } else {
        os << " -> ";
      }
      os << func->getName() << "\n";
    }
    return os;
  }
};

static void push_and_emit(List<FunctionPath> &paths,
                          FunctionPath &path,
                          llvm::Function *func) {
  paths.push_back(path);
  paths.back().push_back(func);
}

static void backtracking_dfs(FunctionPath &path,
                             List<FunctionPath> &paths,
                             Set<llvm::Function *> visited,
                             llvm::CallGraph &callgraph,
                             llvm::CallGraphNode *node,
                             llvm::CallGraphNode *find) {
  // Fetch the function for this node.
  auto *func = node->getFunction();

  // Append this function to the path.
  path.push_back(func);

  // Skip nodes we have already visited.
  if (visited.contains(func)) {

    // If the path is duplicated, stop recursing along it!
    if (path.duplicated()) {
      path.pop_back();
      return;
    }

  } else {
    visited.insert(func);
  }

  // For each outgoing edge, recurse.
  for (auto [call, out] : *node) {
    auto *callee = out->getFunction();

    // Warn about indirect calls.
    if (not callee) {
      continue;
    }

    // If this is a fold instruction, fetch the body as the callee.
    if (call) {
      if (auto *fold = into<FoldInst>((llvm::Value *)*call)) {
        callee = &fold->getBody();
        out = callgraph[callee];
      }
    }

    // Skip calls to external functions and  intrinsics.
    if (callee->empty() or callee->isIntrinsic()) {
      continue;
    }

    // If we have found our destination, add it to the path.
    if (out == find) {
      push_and_emit(paths, path, callee);
      continue;
    }

    // Otherwise, append to the path and recurse.
    backtracking_dfs(path, paths, visited, callgraph, out, find);
  }

  // Perform a quick sanity check that everything was cleaned up before we
  // return.
  MEMOIR_ASSERT(path.back() == func,
                "Misaligned path!\n  Expected ",
                func->getName(),
                "\n  Found ",
                path.back()->getName());

  // Pop this function from the path before we return.
  path.pop_back();

  // Unmark this function as being visited.
  visited.erase(func);
}

static void find_all_paths(List<FunctionPath> &paths,
                           llvm::CallGraph &callgraph,
                           llvm::Function *from,
                           llvm::Function *to) {

  // Fetch the begin and end nodes.
  auto *from_node = callgraph[from];
  auto *to_node = callgraph[to];

  // Initialize an empty path.
  FunctionPath path;

  // Perform a DFS of the callgraph.
  Set<llvm::Function *> visited = {};
  backtracking_dfs(path, paths, visited, callgraph, from_node, to_node);

  return;
}

bool promote_global(llvm::GlobalVariable &global) {
  println("PROMOTE ", global);

  bool modified = false;

  // Fetch module-level information.
  auto &module =
      MEMOIR_SANITIZE(global.getParent(), "Global has no parent module!");

  // Collect the loads and stores to this global.
  Vector<llvm::StoreInst *> stores = {};
  Vector<llvm::LoadInst *> loads = {};

  // Collect the set of functions that the global is used in.
  Set<llvm::Function *> store_functions = {};
  Set<llvm::Function *> load_functions = {};

  // Iterate over the users of the global.
  for (auto *user : global.users()) {
    if (auto *store = dyn_cast<llvm::StoreInst>(user)) {
      stores.push_back(store);
      store_functions.insert(store->getFunction());
    } else if (auto *load = dyn_cast<llvm::LoadInst>(user)) {
      loads.push_back(load);
      load_functions.insert(load->getFunction());
    } else {
      MEMOIR_UNREACHABLE(
          "Unhandled user for global, did you call global_is_promotable prior?");
    }
  }

  // Construct the callgraph for the module.
  llvm::memoir::CallGraph callgraph(module);

  // Determine the store-load function paths.
  List<FunctionPath> paths;
  for (auto *store_func : store_functions) {
    for (auto *load_func : load_functions) {
      // TODO: Refine this by checking the dominator tree, and only considering
      // call edges for calls that do not dominate the store.
      find_all_paths(paths, callgraph, store_func, load_func);
    }
  }

  // Eliminate subset paths.
  for (auto it = paths.begin(); it != paths.end();) {
    auto &outer = *it;
    bool outer_deleted = false;
    for (auto jt = std::next(it); jt != paths.end();) {
      auto &inner = *jt;

      // Check if the inner path is a subset of the outer path.
      if (inner.subset_of(outer)) {
        jt = paths.erase(jt);
        continue;
      }

      // ... and vice versa.
      if (outer.subset_of(inner)) {
        outer_deleted = true;
        it = paths.erase(it);
        break;
      }

      ++jt;
    }
    if (not outer_deleted) {
      ++it;
    }
  }

  // If any of the paths may be entered by an external function call, then we
  // can't transform the function.
  println("FOUND ", paths.size(), " PATH", paths.size() == 1 ? "" : "S");
  for (const auto &path : paths) {
    println(path);
    if (path.called_externally(callgraph)) {
      warnln("PATH MAY BE CALLED EXTERNALLY, CAN'T PROMOTE.");
      return modified;
    }
  }

  return modified;
}

bool promote_globals(llvm::ArrayRef<llvm::GlobalVariable *> globals) {
  bool modified = false;

  for (auto *global : globals) {
    modified |= promote_global(*global);
  }

  return modified;
}

} // namespace llvm::memoir
