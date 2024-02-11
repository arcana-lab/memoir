#ifndef MEMOIR_ANALYSIS_LIVERANGEANALYSIS_H
#define MEMOIR_ANALYSIS_LIVERANGEANALYSIS_H

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MEMOIR
#include "memoir/analysis/RangeAnalysis.hpp"

#include "memoir/support/Graph.hpp"
#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

struct LiveRangeConstraintGraph;

class LiveRangeAnalysis {
public:
  /**
   * Live-Range analysis driver.
   * Constructs a live range valuation for MEMOIR sequence variables in an LLVM
   * module @M using analyses from @noelle. Can toggle context sensitivity
   * with @context_sensitive, defaults to context insesitive.
   */
  LiveRangeAnalysis(llvm::Module &M,
                    arcana::noelle::Noelle &noelle,
                    bool context_sensitive = false)
    : M(M),
      noelle(noelle),
      context_sensitive(context_sensitive) {
    this->run();
  }
  ~LiveRangeAnalysis();

  /**
   * Query the live range for MEMOIR sequence variable @V.
   * If @V is not a MEMOIR sequence variable, returns NULL!
   */
  ValueRange *get_live_range(llvm::Value &V) const;

  /**
   * Query the live range for MEMOIR sequence variable @V in calling context @C.
   * If @V is not a MEMOIR sequence variable, returns NULL!
   */
  ValueRange *get_live_range(llvm::Value &V, llvm::CallBase &C) const;

  /**
   * Acquire the results of the live range analysis.
   */
  const map<llvm::Value *, map<llvm::CallBase *, ValueRange *>> &results()
      const {
    return this->live_ranges;
  }

protected:
  // Analysis driver.
  void run();

  // Analysis steps.
  LiveRangeConstraintGraph construct();
  void evaluate(LiveRangeConstraintGraph &graph);

  // Analysis helpers.

  // Query helpers.
  ValueRange *lookup_live_range(llvm::Value &V, llvm::CallBase *C) const;

  // Owned state.
  map<llvm::Function *, RangeAnalysis *> intraprocedural_range_analyses;

  // Borrowed state.
  map<llvm::Value *, map<llvm::CallBase *, ValueRange *>> live_ranges;

  llvm::Module &M;
  arcana::noelle::Noelle &noelle;
  bool context_sensitive;
};

struct LiveRangeConstraintGraph
  : public DirectedGraph<llvm::Value *, llvm::Use *, ValueRange *> {
public:
  void add_uses_to_graph(RangeAnalysis &RA, llvm::Instruction &I);
  void add_use_to_graph(llvm::Use &U);
  void add_index_use_to_graph(llvm::Use &U, llvm::Value &C);
  void add_index_to_graph(llvm::Value &V, ValueRange &VR);
  void add_seq_to_graph(llvm::Value &V);
};

} // namespace llvm::memoir

#endif // MEMOIR_ANALYSIS_LIVERANGEANALYSIS_H
