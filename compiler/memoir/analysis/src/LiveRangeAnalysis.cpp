#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/analysis/LiveRangeAnalysis.hpp"

namespace llvm::memoir {

// Analysis queries.
ValueRange *LiveRangeAnalysis::lookup_live_range(llvm::Value &V,
                                                 llvm::CallBase *C) const {
  // Lookup the value.
  auto found_value = this->live_ranges.find(&V);
  if (found_value != this->live_ranges.end()) {
    auto &context_sensitive_ranges = found_value->second;
    auto found_context = context_sensitive_ranges.find(C);
    if (found_context != context_sensitive_ranges.end()) {
      return found_context->second;
    }
    auto found_insensitive = context_sensitive_ranges.find(nullptr);
    if (found_insensitive != context_sensitive_ranges.end()) {
      return found_insensitive->second;
    }
  }

  // Failed to find.
  return nullptr;
}

ValueRange *LiveRangeAnalysis::get_live_range(llvm::Value &V) const {
  return this->lookup_live_range(V, nullptr);
}

ValueRange *LiveRangeAnalysis::get_live_range(llvm::Value &V,
                                              llvm::CallBase &C) const {
  return this->lookup_live_range(V, &C);
}

// Analysis steps.
LiveRangeConstraintGraph LiveRangeAnalysis::construct() {
  LiveRangeConstraintGraph graph;
  // For each function in the program.
  for (auto &F : this->M) {
    if (F.empty()) {
      continue;
    }

    // Run the intraprocedural range analysis for the function.
    auto &RA = MEMOIR_SANITIZE(new RangeAnalysis(F, noelle),
                               "Unable to construct range analysis!");
    this->intraprocedural_range_analyses[&F] = &RA;

    // For each sequence variable in the program, add it to the constraints
    // graph.
    set<llvm::Instruction *> visited = {};
    set<llvm::Instruction *> users_to_visit = {};
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (isa_and_nonnull<SequenceType>(TypeAnalysis::analyze(I))) {

          visited.insert(&I);

          // Add the variable to the constraints graph.
          graph.add_seq_to_graph(I);

          // Construct index nodes and use edges for this instruction.
          graph.add_uses_to_graph(RA, I);

          // For uses of this instruction, recurse on them.
          for (auto &use : I.uses()) {
            if (auto *user_as_inst =
                    dyn_cast_or_null<llvm::Instruction>(use.getUser())) {
              users_to_visit.insert(user_as_inst);
            }
          }
        }
      }
    }

    // Handle users of sequence variables.
    for (auto *user : users_to_visit) {
      if (visited.count(user) == 0) {
        graph.add_uses_to_graph(RA, *user);
      }
    }
  }

  debugln("Constraint graph:");
  for (auto *node : graph) {
    debugln(" ├─┬─╸ ", *node);
    debugln(" │ ├─┬─╸ outgoing: ");
    for (auto [out, _] : graph.outgoing(node)) {
      debugln(" │ │ ├─╸ ", *out);
    }
    debugln(" │ │ ╹ ");
    debugln(" │ ├─┬─╸ incoming: ");
    for (auto [in, _] : graph.incoming(node)) {
      debugln(" │ │ ├─╸ ", *in);
    }
    debugln(" │ ╹ ╹ ");
  }
  debugln(" ╹ ");

  // Return the graph.
  return graph;
}

void LiveRangeAnalysis::evaluate(LiveRangeConstraintGraph &graph) {
  // Compute the condensation of the constraint graph.
  auto condensation = graph.condense();

  debugln("Condensation graph:");
  for (auto *node : condensation) {
    debugln(" ├─┬─╸ ", *node);
    debugln(" │ ├─┬─╸ outgoing: ");
    for (auto [out, _] : condensation.outgoing(node)) {
      debugln(" │ │ ├─╸ ", *out);
    }
    debugln(" │ │ ╹ ");
    debugln(" │ ├─┬─╸ incoming: ");
    for (auto [in, _] : condensation.incoming(node)) {
      debugln(" │ │ ├─╸ ", *in);
    }
    debugln(" │ │ ╹ ");
    debugln(" │ ├─┬─╸ : ");
    for (auto *component : condensation.node_prop(node)) {
      debugln(" │ │ ├─╸ ", *component);
    }
    debugln(" │ ╹ ╹ ");
  }
  debugln(" ╹ ");

  // Compute the topographical order of the components.
  auto topological_order = *condensation.topological_order();

  debugln("Topological order: ");
  for (auto *value : topological_order) {
    debugln(" ├─┬─╸ ", *value);
    for (auto *component : condensation.node_prop(value)) {
      if (component != value) {
        debugln(" │ ├─╸ ", *component);
      }
    }
    debugln(" │ ╹ ");
  }
  debugln(" ╹");

  // For each SCC in the condensation graph, in topological order:
  for (auto *root : topological_order) {

    // Evaluate the root of the SCC.
    auto &merged_range = graph.node_prop(root);
    for (auto [in, _] : condensation.incoming(root)) {

      // Get the incoming live range.
      auto *range = graph.node_prop(in);

      // If the range is underdefined, skip it.
      if (range == nullptr) {
        continue;
      }

      println("merging ", *range);

      // Merge the range with the current range.
      // TODO: flesh this out.
      if (merged_range == nullptr) {
        merged_range = range;
      } else {
        MEMOIR_UNREACHABLE("Range merging is unimplemented!");
      }
    }

    // TODO: handle context-sensitivity.

    // Propagate root evaluation along all edges within the SCC.
    auto &scc = condensation.node_prop(root);
    for (auto *node : scc) {
      if (node == root) {
        continue;
      }

      auto &merged_range = graph.node_prop(node);
      for (auto [in, _] : graph.incoming(node)) {

        // Get the incoming live range.
        auto *range = graph.node_prop(in);

        // If the range is underdefined, skip it.
        if (range == nullptr) {
          continue;
        }

        // Merge the range with the current range.
        // TODO: handle different use types.
        if (merged_range == nullptr) {
          merged_range = range;
        } else if (merged_range == range) {
          // Do nothing.
        } else {
          MEMOIR_UNREACHABLE("Range merging is unimplemented!");
        }
      }
    }
  }

  // Print the results.
  debugln("Valuation:");
  for (auto *node : graph) {
    auto *range = graph.node_prop(node);

    this->live_ranges[node][nullptr] = range;

    debugln(*node);
    if (range) {
      debugln("    ==  ", *range);
    } else {
      debugln("    ==  ⊥");
    }
  }

  return;
}

// Analysis driver.
void LiveRangeAnalysis::run() {
  // Construct the constraints graph.
  auto graph = construct();

  // Evaluate the constraints graph.
  evaluate(graph);
}

// Logistics.
LiveRangeAnalysis::~LiveRangeAnalysis() {
  for (auto const &[function, range_analysis] :
       intraprocedural_range_analyses) {
    delete range_analysis;
  }
}

} // namespace llvm::memoir
