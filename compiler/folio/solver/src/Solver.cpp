#include <regex>

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

void Solver::parse_model(clingo_model_t const *model) {
  bool ret = true;
  clingo_symbol_t *atoms = NULL;
  size_t atoms_n;
  clingo_symbol_t const *it, *ie;
  char *str = NULL;
  size_t str_n;

  // Determine the number of (shown) symbols in the model
  if (not clingo_model_symbols_size(model, clingo_show_type_shown, &atoms_n)) {
    warnln("Couldn't determine size of model.");
    return;
  }

  // Allocate required memory to hold all the symbols
  if (not(atoms = (clingo_symbol_t *)malloc(sizeof(*atoms) * atoms_n))) {
    MEMOIR_UNREACHABLE("Could not allocate memory for atoms");
  }

  // Retrieve the symbols in the model
  if (not clingo_model_symbols(model, clingo_show_type_shown, atoms, atoms_n)) {
    warnln("Couldn't retrieve symbols from model.");
    free(atoms);
    return;
  }

  // Create a new candidate.
  this->_candidates.emplace_back();
  auto &candidate = this->_candidates.back();

  // For each atom in the solution, parse it and populate the candidate.
  const std::regex opportunity_regex("use([_[:alnum:]]+)");
  std::smatch regex_match;

  for (it = atoms, ie = atoms + atoms_n; it != ie; ++it) {

    // Get the name of the symbol.
    const char *name_str;
    if (not clingo_symbol_name(*it, &name_str)) {
      warnln("Failed to get name of atom");
      break;
    }
    std::string name(name_str);

    // Get the atom arguments.
    clingo_symbol_t const *arguments;
    size_t arguments_size;
    if (not clingo_symbol_arguments(*it, &arguments, &arguments_size)) {
      warnln("Failed to get arguments of atom");
      break;
    }

    // Parse the atom.
    if (name == "select") {
      // Fetch the value.
      int value_id;
      if (not clingo_symbol_number(arguments[0], &value_id)) {
        warnln("Failed to get value id's string");
        break;
      }
      auto &value = this->_env.lookup(uint32_t(value_id));

      // Fetch the selected implementation.
      const char *selected_str;
      if (not clingo_symbol_name(arguments[1], &selected_str)) {
        warnln("Failed to get selected implementation's string");
        break;
      }
      std::string selected_name(selected_str);
      auto *selected = &this->_implementations.at(selected_name);

      // Add the selection to the candidate.
      candidate._selections[&value] = selected;

    } else if (name.substr(0, 3) == "use") {
      // Fetch the opportunity being used.
      auto opportunity_str = name.substr(3);

      // Fetch the opportunity ID.
      int opportunity_id;
      if (not clingo_symbol_number(arguments[0], &opportunity_id)) {
        warnln("Failed to get opportunity ID.");
      }

    } else {
      warnln("Unknown atom, ", name, ", skipping");
    }
  }

  if (atoms) {
    free(atoms);
  }

  return;
}

void Solver::parse_solution(clingo_solve_handle_t *handle) {
  clingo_model_t const *model;

  // Iterate over all models in the solution.
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
    this->parse_model(model);
  }
}

#define FORMULATE_TYPE(NAME) "type(" #NAME ")"

std::string Solver::formulate() {

  std::string formula = "";

  // First, formulate all primitive types.
  formula += "type(u64). type(u32). type(u16). type(u8)."
             "type(i64). type(i32). type(i16). type(i8)."
             "type(f64). type(f32). type(boolean). type(void)."
             "type(seq). type(assoc).\n";

  // A collection must be typed.
  formula += "typed(C) :- collection(C), assoc(C), "
             "keytype(C, KT), type(KT), "
             "valtype(C, VT), type(VT).\n";
  formula += "typed(C) :- collection(C), seq(C), "
             "valtype(C, VT), type(VT).\n";
  formula += ":- collection(C), not typed(C).";

  // A collection cannot be both a seq and an assoc.
  formula += ":- seq(C), assoc(C).\n";

  // A collection must have a selection.
  formula += "selected(C) :- collection(C), select(C, I), impl(I).\n";
  formula += ":- collection(C), not selected(C).\n";

  // Formulate all of the available selections.
  for (auto &[impl_name, impl] : this->_implementations) {
    // Register that the implementation exists.
    formula += "impl(" + impl.name() + ").\n";

    // Formulate the head atom.
    auto impl_rule = "select(C, " + impl.name() + ") :- ";

    // Formulate the type requirements.
    if (isa<SeqImplementation>(&impl)) {
      impl_rule += "collection(C), seq(C)";
    } else if (isa<AssocImplementation>(&impl)) {
      impl_rule += "collection(C), assoc(C), not valtype(C, void)";
    } else if (isa<SetImplementation>(&impl)) {
      impl_rule += "collection(C), assoc(C), valtype(C, void)";
    } else {
      impl_rule += "collection(C)";
    }

    // Formulate the set of constraints.
    for (auto constraint : impl.constraints()) {
      impl_rule += ", not " + constraint.name() + "(C)";
    }

    // Formulate the constraint that a collection may have only _one_ selection.
    for (auto &[other_name, other_impl] : this->_implementations) {
      // Skip itself.
      if (impl == other_impl) {
        continue;
      }

      impl_rule += ", not select(C, " + other_name + ")";
    }

    impl_rule += ".\n";

    formula += impl_rule;
  }

  // Formulate the constraint that all collections have a selection.
  // auto all_selected = "collection(C) :- ";
  // for (auto impl : this->_implementations) {
  // }

  // Formulate all of the selectable collections and their constraints.
  set<Type *> derived_types = {};
  for (auto *decl : this->_selectable) {
    // Get the type of the variable.
    auto *type = type_of(*decl);

    if (not type) {
      warnln("Selectable declaration with unknown type encountered, skipping.");
      continue;
    }

    // Get the values identifier.
    auto id = this->_env.get_id(*decl);
    auto id_str = std::to_string(id);

    // Formulate the variable's declaration and type information.
    // TODO: extend this to include the key and element type.
    if (auto *seq_type = dyn_cast<SequenceType>(type)) {
      // Unpack the value type.
      auto &val_type = seq_type->getElementType();
      auto val_str = val_type.get_code().value_or("INVALID_TYPE_ERROR");

      // If we have a derived type or a user-defined type, store it so that we
      // can formulate it later.
      if (isa<ReferenceType>(&val_type) or isa<StructType>(&val_type)) {
        derived_types.insert(&val_type);
        val_str = "t_" + val_str;
      }

      // Formulate the collection declaration.
      formula += "collection(" + id_str + "). ";
      formula += "{seq(" + id_str + ")}. ";
      formula += "{valtype(" + id_str + ", " + val_str + ")}. ";

    } else if (auto *assoc_type = dyn_cast<AssocArrayType>(type)) {

      // Unpack the key type.
      auto &key_type = assoc_type->getKeyType();
      auto key_str = key_type.get_code().value_or("INVALID_TYPE_ERROR");
      if (isa<ReferenceType>(&key_type) or isa<StructType>(&key_type)) {
        derived_types.insert(&key_type);
        key_str = "t_" + key_str;
      }

      // Unpack the value type.
      auto &val_type = assoc_type->getValueType();
      auto val_str = val_type.get_code().value_or("INVALID_TYPE_ERROR");
      if (isa<ReferenceType>(&val_type) or isa<StructType>(&val_type)) {
        derived_types.insert(&val_type);
        val_str = "t_" + val_str;
      }

      // Formulate the collection declaration.
      formula += "collection(" + id_str + "). ";
      formula += "{assoc(" + id_str + ")}. ";
      formula += "{keytype(" + id_str + ", " + key_str + ")}. ";
      formula += "{valtype(" + id_str + ", " + val_str + ")}. ";
    }

    // Formulate the value's constraints.
    for (const auto constraint : this->_constraints[*decl]) {
      formula += " " + constraint.name() + "(" + id_str + ").";
    }
    formula += "\n";
  }

  // Formulate all required derived and user-defined types.
  for (auto *type : derived_types) {
    auto type_str = "t_" + type->get_code().value_or("INVALID_TYPE_ERROR");
    formula += "type(" + type_str + ").\n";
  }

  // Formulate all of the opportunities.
  // TODO

  // Specify that selections should be emitted.
  formula += "#show select/2.";

  // Specify that opportunities and their negations should be emitted.
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

  debugln();
  debugln("Formula:");
  debugln(formula);
  debugln();

  //// Solve the ASP problem.
  clingo_control_t *ctl = NULL;
  clingo_part_t parts[] = { { "base", NULL, 0 } };

  //// Create a control object.
  const char *args[] = { "-n", "0" };
  if (not clingo_control_new(args, 2, nullptr, nullptr, 0, &ctl)) {
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
  this->parse_solution(handle);

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

// =========================

} // namespace folio
