#ifndef MEMOIR_ANALYSIS_LIVERANGEANALYSIS_H
#define MEMOIR_ANALYSIS_LIVERANGEANALYSIS_H

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/analysis/RangeAnalysis.hpp"

#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Graph.hpp"

namespace llvm::memoir {

struct LiveRangeAnalysisResult {
public:
  /**
   * Query the live range for MEMOIR sequence variable V.
   * If V is not a MEMOIR sequence variable, returns NULL!
   *
   * @param V Value to query live range of
   * @returns the ValueRange corresponding to the value.
   */
  ValueRange *get_live_range(llvm::Value &V) const;

  /**
   * Query the live range for MEMOIR sequence variable V in calling context C.
   * If V is not a MEMOIR sequence variable, returns NULL!
   *
   * @param V Value to query live range of
   * @param C Calling context for context-sensitive results.
   * @returns the ValueRange corresponding to the value.
   */
  ValueRange *get_live_range(llvm::Value &V, llvm::CallBase &C) const;

  /**
   * Get the live ranges analysed in the module.
   *
   * @returns a nested mapping from LLVM Values and their CallBase context (or
   * NULL) to their ValueRange.
   */
  const Map<llvm::Value *, Map<llvm::CallBase *, ValueRange *>> &live_ranges()
      const;

  friend class LiveRangeAnalysisDriver;

protected:
  ValueRange *lookup_live_range(llvm::Value &V, llvm::CallBase *C) const;

  Map<llvm::Value *, Map<llvm::CallBase *, ValueRange *>> _live_ranges;
};

struct LiveRangeConstraintGraph;

class LiveRangeAnalysisDriver {
public:
  /**
   * Live-Range analysis driver.
   * Constructs a live range valuation for MEMOIR sequence variables in an LLVM
   * module M using analyses from noelle. Can toggle context sensitivity
   * with context_sensitive, defaults to context insesitive.
   *
   * @param M LLVM Module to analyze
   * @param M LLVM Module Analysis Manager
   * @param noelle NOELLE instance
   * @param result The result struct to store everything in
   * @param context_sensitive Toggle whether the analysis should be context
   * sensitive or not.
   */
  LiveRangeAnalysisDriver(llvm::Module &M,
                          RangeAnalysisResult &RA,
                          arcana::noelle::Noelle &noelle,
                          LiveRangeAnalysisResult &result,
                          bool context_sensitive = false)
    : M(M),
      RA(RA),
      noelle(noelle),
      result(result),
      context_sensitive(context_sensitive) {
    this->run();
  }

  /**
   * Perform the disjunctive merge of two value ranges, range1 and range2.
   * Returns the resultant value range.
   */
  static ValueRange *disjunctive_merge(ValueRange *range1, ValueRange *range2);

  /**
   * Perform the conjunctive merge of two value ranges, range1 and range2.
   * Returns the resultant value range.
   */
  static ValueRange *conjunctive_merge(ValueRange *range1, ValueRange *range2);

protected:
  // Analysis driver.
  void run();

  // Analysis steps.
  LiveRangeConstraintGraph construct();
  void evaluate(LiveRangeConstraintGraph &graph);

  // Analysis helpers.

  // Query helpers.
  ValueRange *lookup_live_range(llvm::Value &V, llvm::CallBase *C) const;

  // Borrowed state.
  llvm::Module &M;
  RangeAnalysisResult &RA;
  arcana::noelle::Noelle &noelle;
  LiveRangeAnalysisResult &result;
  bool context_sensitive;

public:
  ~LiveRangeAnalysisDriver() {}
};

using Constraint = std::function<ValueRange *(ValueRange *)>;

struct LiveRangeConstraintGraph
  : public DirectedGraph<llvm::Value *, Constraint, ValueRange *> {
public:
  // Constraints.
  ValueRange *propagate_edge(llvm::Value *from,
                             llvm::Value *to,
                             Constraint constraint);

  // Construction.
  void add_uses_to_graph(RangeAnalysisResult &RA, llvm::Instruction &I);
  void add_use_to_graph(llvm::Use &U, Constraint constraint);
  void add_index_use_to_graph(llvm::Use &U, llvm::Value &C);
  void add_index_to_graph(llvm::Value &V, ValueRange &VR);
  void add_seq_to_graph(llvm::Value &V);
};

} // namespace llvm::memoir

#endif // MEMOIR_ANALYSIS_LIVERANGEANALYSIS_H
