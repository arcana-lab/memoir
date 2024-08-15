
// Clingo
#include "clingo.h"

// MEMOIR
#include "memoir/ir/TypeCheck.hpp"
#include "memoir/ir/Types.hpp"

#include "memoir/support/Print.hpp"

// Folio
#include "folio/solver/Solver.hpp"

using namespace llvm::memoir;

namespace folio {

namespace detail {
bool print_model(clingo_model_t const *model) {
  bool ret = true;
  clingo_symbol_t *atoms = NULL;
  size_t atoms_n;
  clingo_symbol_t const *it, *ie;
  char *str = NULL;
  size_t str_n = 0;

  // determine the number of (shown) symbols in the model
  if (!clingo_model_symbols_size(model, clingo_show_type_shown, &atoms_n)) {
    goto error;
  }

  // allocate required memory to hold all the symbols
  if (!(atoms = (clingo_symbol_t *)malloc(sizeof(*atoms) * atoms_n))) {
    clingo_set_error(clingo_error_bad_alloc,
                     "could not allocate memory for atoms");
    goto error;
  }

  // retrieve the symbols in the model
  if (!clingo_model_symbols(model, clingo_show_type_shown, atoms, atoms_n)) {
    goto error;
  }

  printf("Model:");

  for (it = atoms, ie = atoms + atoms_n; it != ie; ++it) {
    size_t n;
    char *str_new;

    // determine size of the string representation of the next symbol in the
    // model
    if (!clingo_symbol_to_string_size(*it, &n)) {
      goto error;
    }

    if (str_n < n) {
      // allocate required memory to hold the symbol's string
      if (!(str_new = (char *)realloc(str, sizeof(*str) * n))) {
        clingo_set_error(clingo_error_bad_alloc,
                         "could not allocate memory for symbol's string");
        goto error;
      }

      str = str_new;
      str_n = n;
    }

    // retrieve the symbol's string
    if (!clingo_symbol_to_string(*it, str, n)) {
      goto error;
    }

    printf(" %s", str);
  }

  printf("\n");
  goto out;

error:
  ret = false;

out:
  if (atoms) {
    free(atoms);
  }
  if (str) {
    free(str);
  }

  return ret;
}
} // namespace detail

uint32_t Solver::get_id(llvm::Value &V) {
  auto found = this->_value_ids.find(&V);
  if (found != this->_value_ids.end()) {
    return found->second;
  }

  // If we couldn't find an ID, create one.
  auto id = this->_current_id++;
  this->_value_ids[&V] = id;
  this->_id_values[id] = &V;

  return id;
}

std::string Solver::formulate() {

  std::string formula;

  // Instantiate all of the selectable collections.

  // Formulate all of the selectable collections and their constraints.
  for (auto *decl : this->_selectable) {
    // Get the type of the variable.
    auto *type = type_of(*decl);

    if (not type) {
      warnln("Selectable declaration with unknown type encountered, skipping.");
      continue;
    }

    // Get the values identifier.
    auto id = this->get_id(*decl);
    auto id_str = std::to_string(id);

    // Formulate the variable's declaration and type information.
    // TODO: extend this to include the key and element type.
    if (isa<SequenceType>(type)) {
      formula += "seq(" + id_str + "). ";
    } else if (isa<AssocArrayType>(type)) {
      formula += "assoc(" + id_str + "). ";
    }

    // Formulate the value's constraints.
    for (const auto constraint : this->_constraints[*decl]) {
      formula += " " + constraint.name() + "(" + id_str + ").";
    }
    formula += "\n";
  }

  // Formulate all of the opportunities.
  // TODO

  return formula;
}

Solver::Solver(const llvm::memoir::set<llvm::Value *> &selectable,
               Constraints &constraints,
               const Opportunities &opportunities,
               const Implementations &implementations)
  : _selectable{ selectable },
    _constraints{ constraints },
    _opportunities{ opportunities },
    _implementations{ implementations } {

  // Formulate the ASP problem.
  std::string formula = this->formulate();

  println();
  println("Formula:");
  println(formula);
  println();

  //// Solve the ASP problem.
  clingo_control_t *ctl = NULL;
  clingo_part_t parts[] = { { "base", NULL, 0 } };

  //// Create a control object.
  const char *args[] = { "-n", "0" };
  if (not clingo_control_new(args, 2, nullptr, nullptr, 20, &ctl)) {
    MEMOIR_UNREACHABLE("Failed to create a clingo control.");
  }

  //// Add the logic formula to the base part.
  if (not clingo_control_add(ctl, "base", nullptr, 0, formula.c_str())) {
    MEMOIR_UNREACHABLE("Failed to add formula to clingo.");
  }

  //// Ground the base part.
  if (not clingo_control_ground(ctl, parts, 1, nullptr, nullptr)) {
    MEMOIR_UNREACHABLE("Failed to ground formula.");
  }

  // Solve.
  clingo_solve_result_bitset_t solve_ret;

  //// Solve and get the solve handle.
  clingo_solve_handle_t *handle;
  if (not clingo_control_solve(ctl,
                               clingo_solve_mode_yield,
                               nullptr,
                               0,
                               nullptr,
                               nullptr,
                               &handle)) {
    MEMOIR_UNREACHABLE("Failed to get a solve handle.");
  }

  // Parse the solution to populate the candidate list.
  clingo_model_t const *model;

  //// Iterate over all models.
  while (true) {
    if (not clingo_solve_handle_resume(handle)) {
      MEMOIR_UNREACHABLE("Failed to go to next model in solve handle.");
    }
    if (not clingo_solve_handle_model(handle, &model)) {
      MEMOIR_UNREACHABLE("Failed to get model from solve handle");
    }

    // If there are no more models, break.
    if (not model) {
      break;
    }

    // Parse the model.
    // TODO: update print to parse to a data structure.
    detail::print_model(model);
  }

  // Close the solve handle.
  if (not clingo_solve_handle_get(handle, &solve_ret)) {
    MEMOIR_UNREACHABLE("Failed to close solve handle.");
  }

  // Free the control.
  if (ctl) {
    clingo_control_free(ctl);
  }
}

// =========================
// Query operations.

const Candidates &Solver::candidates() const {
  return this->_candidates;
}

llvm::Value &Solver::lookup(uint32_t id) const {
  auto found = this->_id_values.find(id);
  if (found == this->_id_values.end()) {
    MEMOIR_UNREACHABLE("Invalid ID");
  }

  return *(found->second);
}

// =========================

} // namespace folio
