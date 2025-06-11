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

#include "folio/transforms/Candidate.hpp"
#include "folio/transforms/ObjectInfo.hpp"

namespace folio {

struct ProxyInsertion {
public:
  using GetDominatorTree =
      std::function<llvm::DominatorTree &(llvm::Function &)>;
  using GetBoundsChecks =
      std::function<llvm::memoir::BoundsCheckResult &(llvm::Function &)>;

  ProxyInsertion(llvm::Module &M,
                 GetDominatorTree get_dominator_tree,
                 GetBoundsChecks get_bounds_checks);

  void analyze();

  void prepare();

  bool transform();

protected:
  void gather_assoc_objects(llvm::memoir::Vector<ObjectInfo> &allocations,
                            llvm::memoir::AllocInst &alloc,
                            llvm::memoir::Type &type,
                            llvm::memoir::Vector<unsigned> offsets = {});

  ObjectInfo *find_base_object(llvm::Value &V,
                               llvm::memoir::AccessInst &access);

  void gather_propagators(
      llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Value *>>
          encoded,
      llvm::memoir::Map<llvm::Function *, llvm::memoir::Set<llvm::Use *>>
          to_decode);

  llvm::Module &M;
  llvm::memoir::Vector<ObjectInfo> objects;
  llvm::memoir::Vector<ObjectInfo> propagators;
  llvm::memoir::Vector<Candidate> candidates;

  GetDominatorTree get_dominator_tree;
  GetBoundsChecks get_bounds_checks;
};

} // namespace folio

#endif // FOLIO_TRANSFORMS_PROXYINSERTION_H
