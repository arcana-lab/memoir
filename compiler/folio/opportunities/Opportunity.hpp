#ifndef FOLIO_OPPORTUNITY_H
#define FOLIO_OPPORTUNITY_H

#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "memoir/support/InternalDatatypes.hpp"

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
  static std::string formulate();

  /**
   * Formulate this opportunity instance as a logic program.
   *
   * @return the formula as a string
   */
  virtual std::string formulate() const = 0;

  /**
   * Transform the program to exploit this opportunity.
   *
   * @return true if the program was modified, false otherwise.
   */
  virtual bool exploit() = 0;

  virtual ~Opportunity() = 0;
};

using Opportunities = typename llvm::memoir::vector<Opportunity *>;

} // namespace folio

#endif // FOLIO_OPPORTUNITY_H
