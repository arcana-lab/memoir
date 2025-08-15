#ifndef FOLIO_TRANSFORMS_PROXYINSERTION_H
#define FOLIO_TRANSFORMS_PROXYINSERTION_H

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"

#include "folio/Candidate.hpp"
#include "folio/ObjectInfo.hpp"
#include "folio/Utilities.hpp"

namespace folio {

using llvm::memoir::AllocInst;
using llvm::memoir::Type;

struct ProxyInsertion {
public:
  // Helper types.
  using GetDominatorTree =
      std::function<llvm::DominatorTree &(llvm::Function &)>;
  using GetBoundsChecks =
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>;
  using Enumerated = Map<NestedObject, SmallSet<Candidate *, 1>>;

  // Constructors.
  ProxyInsertion(llvm::Module &M,
                 GetDominatorTree get_dominator_tree,
                 GetBoundsChecks get_bounds_checks);

  // Driver functions.
  void analyze();

  void optimize();

  void prepare();

  bool transform();

  // Helper functions.
  static Option<std::string> get_enumerated_impl(Type &type,
                                                 bool is_nested = false);

protected:
  void gather_assoc_objects();
  void gather_assoc_objects(AllocInst &alloc);
  void gather_assoc_objects(AllocInst &alloc, Type &type, Offsets offsets = {});

  void gather_propagators();
  void gather_propagators(const Set<Type *> &types, AllocInst &alloc);
  void gather_propagators(const Set<Type *> &types,
                          AllocInst &alloc,
                          Type &type,
                          OffsetsRef offsets = {});

  void gather_abstract_objects();
  void gather_abstract_objects(ObjectInfo &object);

  void flesh_out(Candidate &arguments);

  void share_proxies();

  ObjectInfo *find_base_object(llvm::Value &V,
                               llvm::memoir::AccessInst &access);

  llvm::Module &M;
  Vector<BaseObjectInfo> objects, propagators;
  Vector<ArgObjectInfo> arguments;
  Vector<Candidate> candidates;
  Enumerated enumerated;

  GetDominatorTree get_dominator_tree;
  GetBoundsChecks get_bounds_checks;
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_PROXYINSERTION_H
