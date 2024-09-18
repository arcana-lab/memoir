#include "llvm/IR/Value.h"

#include "memoir/support/InternalDatatypes.hpp"

namespace folio {

/**
 * The FormulaEnvironment stores the mappings from ASP tokens
 * to LLVM objects.
 */
struct FormulaEnvironment {
public:
  /**
   * Lookup the LLVM value for the given LLVM value.
   *
   * @param id the identifier
   * @returns the corresponding LLVM Value
   */
  llvm::Value &lookup(uint32_t id) const;

  /**
   * Get the identifier for the given LLVM Value, if it does not already exist,
   * create a unique identifier for it.
   *
   * @param V the LLVM Value
   * @returns the corresponding identifier
   */
  uint32_t get_id(llvm::Value &V);

  llvm::Module &module() const;

  FormulaEnvironment(llvm::Module &M) : M(M) {}

protected:
  llvm::Module &M;
  llvm::memoir::map<llvm::Value *, uint32_t> _value_ids = {};
  llvm::memoir::map<uint32_t, llvm::Value *> _id_values = {};
  uint32_t _current_id = 0;
};
} // namespace folio
