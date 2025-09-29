#include <algorithm>

#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/CallGraph.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/transform/utilities/PromoteGlobals.hpp"
#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

namespace memoir {

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

    // If we reached the beginning, then we are only duplicated if we also
    // reached the start of the search.
    if (jt == je and it != ie) {
      return false;
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
        os << "\n -> ";
      }
      os << func->getName();
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

static llvm::Instruction *insertion_point(llvm::Function *func) {
  auto &entry = func->getEntryBlock();
  return entry.getFirstNonPHIOrDbg();
}

bool promote_global(llvm::GlobalVariable &global) {

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
  memoir::CallGraph callgraph(module);

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

  // Collect all of the functions in the paths
  Set<llvm::Function *> on_path, in_path;

  // If any of the paths may be entered by an external function call, then we
  // can't transform the function.
  for (const auto &path : paths) {
    debugln(path);
    if (path.called_externally(callgraph)) {
      warnln("PATH MAY BE CALLED EXTERNALLY, CAN'T PROMOTE.");
      return modified;
    }

    // Collect internal functions on this path.
    on_path.insert(path.begin(), path.end());
    in_path.insert(std::next(path.begin()), path.end());
  }

  // Collect the calls that need to be patched.
  struct Patch {
    Set<llvm::CallBase *> load, init;
    llvm::GlobalVariable *global;
    llvm::AllocaInst *local;
    Vector<llvm::StoreInst *> stores;
    Vector<llvm::LoadInst *> loads;

    void to_load(llvm::CallBase *call) {
      this->load.insert(call);
    }

    void to_init(llvm::CallBase *call) {
      this->init.insert(call);
    }
  };

  // Version functions with function calls outside this path.
  // TODO: this would be easier with a reverse call graph.
  Map<llvm::Function *, Patch> calls_to_patch = {};
  Vector<llvm::Function *> to_version = {};
  for (auto *func : in_path) {

    // If the function is a fold body.
    if (auto *fold = FoldInst::get_single_fold(*func)) {
      auto *call = &fold->getCallInst();
      auto *caller = fold->getFunction();
      if (not on_path.contains(caller)) {
        calls_to_patch[func].to_init(call);
      } else {
        calls_to_patch[func].to_load(call);
      }

      continue;
    }

    // If the function may be externally called, we need to version it.
    auto callers = possible_callers(*func);
    if (has_unknown_caller(callers)) {
      to_version.push_back(func);
      continue;
    }

    for (auto *call : callers) {
      // Skip indirect calls.
      if (call->isIndirectCall()) {
        continue;
      }

      // We need to version functions with off-path callers.
      auto *caller = call->getFunction();
      if (not on_path.contains(caller)) {
        calls_to_patch[func].to_init(call);
        continue;
      }

      // We need to patch any call in the path.
      calls_to_patch[func].to_load(call);
    }
  }

  // Version functions as needed.
  for (auto *func : to_version) {
    // TODO: Clone the function for the path.
    warnln("UNIMPLEMENTED: ", func->getName(), " NEEDS TO BE VERSIONED");
    return modified;
  }

  // Unpack the global.
  auto *type = global.getValueType();
  auto *init = global.getInitializer();
  llvm::Twine name("");
  if (global.hasName()) {
    name.concat(global.getName());
  }

  //  Collect the local load/stores for the global.
  for (auto &use : global.uses()) {
    auto *user = dyn_cast<llvm::Instruction>(use.getUser());
    if (not user) {
      continue;
    }

    auto *func = user->getFunction();
    if (not func) {
      continue;
    }

    auto &patch = calls_to_patch[func];

    if (auto *store = dyn_cast<llvm::StoreInst>(user)) {
      patch.stores.push_back(store);
    } else if (auto *load = dyn_cast<llvm::LoadInst>(user)) {
      patch.loads.push_back(load);
    }
  }

  // Patch calls to each function.
  for (auto &[func, patch] : calls_to_patch) {

    MemOIRBuilder builder(insertion_point(func));

    // Create a temparg in the function.
    patch.global = new llvm::GlobalVariable(
        module,
        type,
        /* constant? */ false,
        llvm::GlobalValue::LinkageTypes::InternalLinkage,
        init,
        name.concat(".temparg"));

    // Create a stack variable in the function.
    patch.local =
        builder.CreateAlloca(type, /* addrspace */ 0, name.concat(".local"));

    // Load the temparg in the function.
    auto *load_temparg = builder.CreateLoad(type, patch.global);
    Metadata::get_or_add<TempArgumentMetadata>(*load_temparg);

    // Store the loaded temparg to the stack.
    builder.CreateStore(load_temparg, patch.local);

    // Replace all loads from the global with loads from the stack variable.
    for (auto *load : patch.loads) {
      auto &ptr_use =
          load->getOperandUse(llvm::LoadInst::getPointerOperandIndex());
      ptr_use.set(patch.local);
    }

    // Replace all stores to the global with stores to the stack variable.
    for (auto *store : patch.stores) {
      auto &ptr_use =
          store->getOperandUse(llvm::StoreInst::getPointerOperandIndex());
      ptr_use.set(patch.local);
    }

    modified |= true;
  }

  // At each call site:
  for (const auto &[func, patch] : calls_to_patch) {

    MemOIRBuilder builder(insertion_point(func));

    for (auto *call : patch.load) {

      builder.SetInsertPoint(call);

      // Load the local for this function.
      auto *caller = call->getFunction();
      auto *local = calls_to_patch.at(caller).local;
      auto *load = builder.CreateLoad(type, local);

      // Store the local to the temparg.
      auto *store_temparg = builder.CreateStore(load, patch.global);
      Metadata::get_or_add<TempArgumentMetadata>(*store_temparg);

      modified |= true;
    }

    for (auto *call : patch.init) {

      builder.SetInsertPoint(call);

      // Store the initial value to the temparg.
      auto *store_temparg = builder.CreateStore(init, patch.global);
      Metadata::get_or_add<TempArgumentMetadata>(*store_temparg);

      modified |= true;
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

} // namespace memoir
