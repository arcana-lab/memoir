#ifndef FOLIO_SOLVER_H
#define FOLIO_SOLVER_H

// Clingo
#include <clingo.h>

// MEMOIR
#include "memoir/support/InternalDatatypes.hpp"

// Folio
#include "folio/analysis/ConstraintInference.hpp"
#include "folio/analysis/OpportunityDiscovery.hpp"

#include "folio/solver/Implementation.hpp"

namespace folio {

struct Candidate {
public:
  /**
   * Instantiate a new, empty candidate.
   */
  Candidate() : _selections{}, _opportunities{} {}

  /**
   * Get the mapping from llvm values to their selected implementations.
   */
  const llvm::memoir::map<llvm::Value *, const Implementation *> selections()
      const {
    return this->_selections;
  }

  /**
   * Get the set of opportunities exploited by this candidate.
   */
  const llvm::memoir::set<Opportunity *> opportunities() const {
    return this->_opportunities;
  }

protected:
  llvm::memoir::map<llvm::Value *, const Implementation *> _selections;
  llvm::memoir::set<Opportunity *> _opportunities;

  friend class Solver;
};

using Candidates = typename llvm::memoir::list<Candidate>;

/**
 * The Solver formulates the constraints, opportunities, implementations as an
 * Answer Set Programming problem and produces a list of candidates selections.
 */
class Solver {
public:
  /**
   * Solve for all possible candidates.
   *
   * @param selectable the set of selectable variable declarations
   * @param constraints the set of constraints
   * @param opportunities the opportunities that can be exploited
   * @param implementations the set of available implementations
   */
  Solver(llvm::Module &M,
         const llvm::memoir::set<llvm::Value *> &selectable,
         Constraints &constraints,
         const Opportunities &opportunities,
         const Implementations &implementations);

  /**
   * Get a reference to the generated candidates.
   *
   * @returns the generated candidates
   */
  const Candidates &candidates() const;

protected:
  // Helper functions.
  std::string formulate();
  uint32_t get_id(llvm::Value &V);

  void parse_model(clingo_model_t const *model);
  void parse_solution(clingo_solve_handle_t *handle);

  // Results.
  Candidates _candidates;

  // Owned state.
  FormulaEnvironment _env;

  // Borrowed state.
  const llvm::memoir::set<llvm::Value *> &_selectable;
  Constraints &_constraints;
  const Opportunities &_opportunities;
  const Implementations &_implementations;
};

} // namespace folio

#endif // FOLIO_SOLVER_H
