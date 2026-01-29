#include "memoir/analysis/LiveRangeAnalysis.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

namespace memoir {

// Constraint functions.
static ValueRange *propagate_range(ValueRange *range) {
  return range;
}

// Constraint application.
ValueRange *LiveRangeConstraintGraph::propagate_edge(llvm::Value *from,
                                                     llvm::Value *to,
                                                     Constraint constraint) {
  // Get the value range of @from.
  auto *from_range = this->node_prop(from);

  // Apply the constraint and return.
  return constraint(from_range);
}

// Graph construction.
void LiveRangeConstraintGraph::add_index_to_graph(llvm::Value &V,
                                                  ValueRange &VR) {
  // Add the node to the graph with its node property set to its value range.
  this->add_node_with_prop(&V, &VR);

  return;
}

void LiveRangeConstraintGraph::add_seq_to_graph(llvm::Value &V) {
  // Add the node to the graph with an unspecified value range.
  this->add_node_with_prop(&V, nullptr);

  return;
}

void LiveRangeConstraintGraph::add_use_to_graph(llvm::Use &U,
                                                Constraint constraint) {
  // Get the value being used.
  auto *used_value = U.get();

  // Get the user.
  auto *user = U.getUser();

  // Add nodes to graph.
  this->add_seq_to_graph(*used_value);
  this->add_seq_to_graph(*user);

  // Add an edge from the value being used to the user to the graph.
  MEMOIR_ASSERT(this->add_edge_with_prop(user, used_value, constraint),
                "Failed to add edge!");

  return;
}

void LiveRangeConstraintGraph::add_index_use_to_graph(llvm::Use &index_use,
                                                      llvm::Value &collection) {

  // Add the collection to the graph if not already there.
  this->add_seq_to_graph(collection);

  // Get the value being used.
  auto *index_value = index_use.get();

  // Add an edge from the value being used to the user to the graph.
  this->add_edge_with_prop(index_value, &collection, propagate_range);

  return;
}

void LiveRangeConstraintGraph::add_uses_to_graph(RangeAnalysisResult &RA,
                                                 llvm::Instruction &I) {

  // Handle MEMOIR instructions.
  if (auto *memoir = into<MemOIRInst>(I)) {

    // For indexed operations, construct their indices as nodes in the graph and
    // the proper constraints edge.
    if (auto *read = dyn_cast<ReadInst>(memoir)) {

      auto *type = &read->getObjectType();

      for (auto &index_use : read->index_operands()) {

        // Only track usage of sequence indices.
        if (isa<SequenceType>(type)) {
          // Fetch the index range.
          auto &index_range = RA.get_value_range(index_use);

          // Add the index to the graph.
          this->add_index_to_graph(
              MEMOIR_SANITIZE(index_use.get(), "Index being used is NULL!"),
              index_range);

          // Add edge for index use.
          this->add_index_use_to_graph(index_use, read->getObject());
        }

        // TODO: Update the constraint graph to handle nested facts.
        break;
      }

    } else if (auto *use_phi = dyn_cast<UsePHIInst>(memoir)) {

      // Add an edge from this value to its sequence operand.
      this->add_use_to_graph(use_phi->getUsedAsUse(), propagate_range);

    } else if (auto *write = dyn_cast<WriteInst>(memoir)) {

      if (not isa<SequenceType>(&write->getInnerObjectType()))
        return;

      // Add an edge from this value to its sequence operand.
      this->add_use_to_graph(write->getObjectAsUse(), propagate_range);

    } else if (auto *insert = dyn_cast<InsertInst>(memoir)) {

      if (not isa<SequenceType>(&insert->getInnerObjectType()))
        return;

      if (auto input_kw = insert->get_keyword<InputKeyword>()) {
        // Add edge for input collection.
        this->add_use_to_graph(
            input_kw->getInputAsUse(),
            [](ValueRange *in) -> ValueRange * {
              MEMOIR_UNREACHABLE("Insert constraint unimplemented!");
            });
      }

      // Add edge for collection.
      this->add_use_to_graph(
          insert->getObjectAsUse(),
          [](ValueRange *in) -> ValueRange * {
            MEMOIR_UNREACHABLE("Insert constraint unimplemented!");
          });

    } else if (auto *remove = dyn_cast<RemoveInst>(memoir)) {

      if (not isa<SequenceType>(&insert->getInnerObjectType()))
        return;

      this->add_use_to_graph(
          remove->getObjectAsUse(),
          [](ValueRange *in) -> ValueRange * {
            MEMOIR_UNREACHABLE("Remove constraint unimplemented!");
          });

    } else if (auto *copy = dyn_cast<CopyInst>(memoir)) {

      if (not isa<SequenceType>(&copy->getInnerObjectType()))
        return;

      // If this is a whole-collection copy, just propagate.
      auto range_kw = copy->get_keyword<RangeKeyword>();
      if (not range_kw) {
        this->add_use_to_graph(copy->getObjectAsUse(), propagate_range);
        return;
      }

      // Get the start index for the copy.
      auto &start_index = range_kw->getBeginAsUse();

      // Get the value range of the start index.
      auto &start_range = RA.get_value_range(start_index);

      this->add_use_to_graph(
          copy->getObjectAsUse(),
          [&start_range](ValueRange *in) -> ValueRange * {
            // If the input is NULL, return NULL.
            if (in == nullptr)
              return nullptr;

            // Compute the addition of the start index and
            // the incoming range.
            auto *lower_inc = new BasicExpression(
                llvm::Instruction::BinaryOps::Add,
                { &start_range.get_lower(), &in->get_lower() });
            auto *upper_inc = new BasicExpression(
                llvm::Instruction::BinaryOps::Add,
                { &start_range.get_upper(), &in->get_upper() });

            return new ValueRange(*lower_inc, *upper_inc);
          });

    } else if (auto *clear = dyn_cast<ClearInst>(memoir)) {
      this->add_use_to_graph(
          clear->getObjectAsUse(),
          [](ValueRange *in) -> ValueRange * {
            MEMOIR_UNREACHABLE("Clear constraint unimplemented!");
          });
    }
  }
  // Handle LLVM instructions.
  else {
    if (auto *phi = dyn_cast<llvm::PHINode>(&I)) {
      // For each incoming operand to the PHI, add its use to the graph.
      for (auto &incoming : phi->incoming_values()) {
        this->add_use_to_graph(incoming, propagate_range);
      }
    }
  }

  return;
}

} // namespace memoir
