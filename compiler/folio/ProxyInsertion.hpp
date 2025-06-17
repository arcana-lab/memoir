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

struct ProxyInsertion {
public:
  // Helper types.
  using GetDominatorTree =
      std::function<llvm::DominatorTree &(llvm::Function &)>;
  using GetBoundsChecks =
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>;

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
  static Option<std::string> get_enumerated_impl(llvm::memoir::Type &type,
                                                 bool is_nested = false);

protected:
  void gather_assoc_objects(Vector<ObjectInfo> &allocations,
                            llvm::memoir::AllocInst &alloc,
                            llvm::memoir::Type &type,
                            Vector<unsigned> offsets = {});

  ObjectInfo *find_base_object(llvm::Value &V,
                               llvm::memoir::AccessInst &access);

  llvm::Module &M;
  Vector<ObjectInfo> objects;
  Vector<ObjectInfo> propagators;
  Vector<Candidate> candidates;

  GetDominatorTree get_dominator_tree;
  GetBoundsChecks get_bounds_checks;
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_PROXYINSERTION_H
