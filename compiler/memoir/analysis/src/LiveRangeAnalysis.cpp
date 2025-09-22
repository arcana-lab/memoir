#include "noelle/core/NoellePass.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/analysis/LiveRangeAnalysis.hpp"

namespace memoir {

// Result queries.
ValueRange *LiveRangeAnalysisResult::lookup_live_range(
    llvm::Value &V,
    llvm::CallBase *C) const {
  // Lookup the value.
  auto found_value = this->_live_ranges.find(&V);
  if (found_value != this->_live_ranges.end()) {
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

ValueRange *LiveRangeAnalysisResult::get_live_range(llvm::Value &V) const {
  return this->lookup_live_range(V, nullptr);
}

ValueRange *LiveRangeAnalysisResult::get_live_range(llvm::Value &V,
                                                    llvm::CallBase &C) const {
  return this->lookup_live_range(V, &C);
}

const Map<llvm::Value *, Map<llvm::CallBase *, ValueRange *>> &
LiveRangeAnalysisResult::live_ranges() const {
  return this->_live_ranges;
}

// Analysis steps.
LiveRangeConstraintGraph LiveRangeAnalysisDriver::construct() {

  // Construct an empty constraint graph.
  LiveRangeConstraintGraph graph;

  // For each function in the program.
  for (auto &F : this->M) {
    if (F.empty()) {
      continue;
    }

    // For each sequence variable in the program, add it to the constraints
    // graph.
    Set<llvm::Instruction *> visited = {};
    Set<llvm::Instruction *> users_to_visit = {};
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (isa_and_nonnull<SequenceType>(type_of(I))
            || isa_and_nonnull<ReadInst>(into<MemOIRInst>(I))) {

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

void LiveRangeAnalysisDriver::evaluate(LiveRangeConstraintGraph &graph) {
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
    debugln(" │ ├─┬─╸ components: ");
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
    for (auto [in, e] : condensation.incoming(root)) {

      // Get the incoming live range.
      auto *range = graph.propagate_edge(in, root, e.prop());

      // If the range is underdefined, skip it.
      if (range == nullptr) {
        continue;
      }

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
      for (auto [in, e] : graph.incoming(node)) {

        // Get the incoming live range.
        auto *range = graph.propagate_edge(in, node, e.prop());

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

    this->result._live_ranges[node][nullptr] = range;

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
void LiveRangeAnalysisDriver::run() {
  // Construct the constraints graph.
  auto graph = construct();

  // Evaluate the constraints graph.
  evaluate(graph);
}

// Operators.
static ValueExpression &create_min(ValueExpression &expr1,
                                   ValueExpression &expr2) {
  return *(new SelectExpression(
      new ICmpExpression(llvm::CmpInst::Predicate::ICMP_ULE, expr1, expr2),
      &expr1,
      &expr2));
}

static ValueExpression &create_max(ValueExpression &expr1,
                                   ValueExpression &expr2) {
  return *(new SelectExpression(
      new ICmpExpression(llvm::CmpInst::Predicate::ICMP_UGE, expr1, expr2),
      &expr1,
      &expr2));
}

ValueRange *LiveRangeAnalysisDriver::disjunctive_merge(ValueRange *range1,
                                                       ValueRange *range2) {

  // If either range is NULL, return the other.
  if (range1 == nullptr) {
    return range2;
  }
  if (range2 == nullptr) {
    return range1;
  }

  // Unpack the value ranges.
  auto &upper1 = range1->get_upper();
  auto &lower1 = range1->get_lower();
  auto &upper2 = range2->get_upper();
  auto &lower2 = range2->get_lower();

  // Construct the disjunctive merge of the two ranges.
  auto &lower_min = create_min(lower1, lower2);

  auto &upper_max = create_max(upper1, upper2);

  // Construct the new value range.
  auto *new_range = new ValueRange(lower_min, upper_max);

  // Return.
  return new_range;
}

ValueRange *LiveRangeAnalysisDriver::conjunctive_merge(ValueRange *range1,
                                                       ValueRange *range2) {
  // If either range is NULL, return the other.
  if (range1 == nullptr) {
    return range2;
  }
  if (range2 == nullptr) {
    return range1;
  }

  // Unpack the value ranges.
  auto &upper1 = range1->get_upper();
  auto &lower1 = range1->get_lower();
  auto &upper2 = range2->get_upper();
  auto &lower2 = range2->get_lower();

  // Construct the conjunctive merge of the two ranges.
  auto &lower_max = create_max(lower1, lower2);

  auto &upper_min = create_min(upper1, upper2);

  // Construct the new value range.
  auto *new_range = new ValueRange(lower_max, upper_min);

  // Return.
  return new_range;
}

// Top-level Analysis.
LiveRangeAnalysisResult LiveRangeAnalysis::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {

  // Construct a new result.
  LiveRangeAnalysisResult result;

  // Fetch NOELLE.
  auto &NOELLE = MAM.getResult<arcana::noelle::NoellePass>(M);

  // Fetch RangeAnalysisResult.
  auto &RA = MAM.getResult<RangeAnalysis>(M);

  // Construct a LiveRangeAnalysisDriver
  LiveRangeAnalysisDriver LRA(M, RA, NOELLE, result);

  return result;
}

llvm::AnalysisKey LiveRangeAnalysis::Key;

} // namespace memoir
