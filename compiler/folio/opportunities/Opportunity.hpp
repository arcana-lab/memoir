#ifndef FOLIO_OPPORTUNITY_H
#define FOLIO_OPPORTUNITY_H

#include "llvm/IR/PassManager.h"

#include "memoir/support/InternalDatatypes.hpp"

#include "folio/solver/FormulaEnvironment.hpp"
#include "folio/solver/Selection.hpp"

namespace folio {

/**
 * The Opportunity interface.
 * This interface describes the base functionality required of an opportunity:
 *  - Formulable
 *  - Exploitable
 */
struct Opportunity {
public:
  /**
   * Formulate the opportunity class as a logic program.
   *
   * @return the formula as a string.
   */
  template <typename O,
            std::enable_if_t<std::is_base_of_v<Opportunity, O>, bool> = true>
  static std::string formulate(FormulaEnvironment &env);

  /**
   * Formulate this opportunity instance as a logic program.
   * This function returns two values, the head of the rule indicating that this
   * opportunity will be used and the rules and facts to add to the formula.
   *
   * @return a pair, containing the head of the opportunity and the formula
   */
  virtual std::pair<std::string, std::string> formulate(
      FormulaEnvironment &env) = 0;

  /**
   * Transform the program to exploit this opportunity.
   *
   * @param selection the lowering target.
   * @return true if the program was modified, false otherwise.
   */
  virtual bool exploit(std::function<Selection &(llvm::Value &)> get_selection,
                       llvm::ModuleAnalysisManager &MAM) = 0;

  virtual ~Opportunity() = 0;
};

using Opportunities = typename llvm::memoir::vector<Opportunity *>;

} // namespace folio

#endif // FOLIO_OPPORTUNITY_H
