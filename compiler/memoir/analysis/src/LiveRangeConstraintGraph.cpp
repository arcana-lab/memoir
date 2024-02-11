#include "memoir/analysis/LiveRangeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

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

void LiveRangeConstraintGraph::add_use_to_graph(llvm::Use &U) {
  // Get the value being used.
  auto *used_value = U.get();

  // Get the user.
  auto *user = U.getUser();

  // Add nodes to graph.
  this->add_seq_to_graph(*used_value);
  this->add_seq_to_graph(*user);

  // Add an edge from the value being used to the user to the graph.
  MEMOIR_ASSERT(this->add_edge_with_prop(user, used_value, &U),
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
  this->add_edge_with_prop(index_value, &collection, &index_use);

  return;
}

void LiveRangeConstraintGraph::add_uses_to_graph(RangeAnalysis &RA,
                                                 llvm::Instruction &I) {

  // Handle MEMOIR instructions.
  if (auto *memoir_inst = as<MemOIRInst>(&I)) {

    // For indexed operations, construct their indices as nodes in the graph and
    // the proper constraints edge.
    if (auto *read_inst = dyn_cast<IndexReadInst>(memoir_inst)) {
      if (read_inst->getNumberOfDimensions() > 1) {
        warnln(
            "Live range analysis of multi-dimensional collections is unsupported!");
        return;
      }

      // Get the index use.
      auto &index_use = read_inst->getIndexOfDimensionAsUse(0);

      // Fetch the index range.
      auto &index_range = RA.get_value_range(index_use);

      // Add the index to the graph.
      this->add_index_to_graph(
          MEMOIR_SANITIZE(index_use.get(), "Index being used is NULL!"),
          index_range);

      // Add edge for index use.
      this->add_index_use_to_graph(index_use, read_inst->getObjectOperand());

    } else if (auto *write_inst = dyn_cast<IndexWriteInst>(memoir_inst)) {
      // Add an edge from this value to its sequence operand.
      this->add_use_to_graph(write_inst->getObjectOperandAsUse());

    } else if (auto *get_inst = dyn_cast<IndexGetInst>(memoir_inst)) {
      if (get_inst->getNumberOfDimensions() > 1) {
        warnln(
            "Live range analysis of multi-dimensional collections is unsupported!");
        return;
      }

      // Get the index use.
      auto &index_use = get_inst->getIndexOfDimensionAsUse(0);

      // Fetch the index range.
      auto &index_range = RA.get_value_range(index_use);

      // Add the index to the graph.
      this->add_index_to_graph(
          MEMOIR_SANITIZE(index_use.get(), "Index being used is NULL!"),
          index_range);

      // Add edge for index use.
      this->add_index_use_to_graph(index_use, get_inst->getObjectOperand());

    } else if (auto *insert_inst = dyn_cast<SeqInsertInst>(memoir_inst)) {
      // Add edge for collection used.
      this->add_use_to_graph(insert_inst->getBaseCollectionAsUse());

    } else if (auto *insert_seq_inst =
                   dyn_cast<SeqInsertSeqInst>(memoir_inst)) {
      // Add edge for collections used.
      this->add_use_to_graph(insert_seq_inst->getBaseCollectionAsUse());
      this->add_use_to_graph(insert_seq_inst->getInsertedCollectionAsUse());

    } else if (auto *remove_inst = dyn_cast<SeqRemoveInst>(memoir_inst)) {
      // Add edge for collection used.
      this->add_use_to_graph(remove_inst->getBaseCollectionAsUse());

    } else if (auto *swap_inst = dyn_cast<SeqSwapInst>(memoir_inst)) {
      // Add edge for _from_ collection.
      this->add_use_to_graph(swap_inst->getFromCollectionAsUse());

      // Add edge for _to_ collection.
      this->add_use_to_graph(swap_inst->getToCollectionAsUse());

    } else if (auto *swap_within_inst =
                   dyn_cast<SeqSwapWithinInst>(memoir_inst)) {
      // Add edge for collection.
      this->add_use_to_graph(swap_within_inst->getFromCollectionAsUse());

    } else if (auto *copy_inst = dyn_cast<SeqCopyInst>(memoir_inst)) {
      // Add edge for copied collection.
      this->add_use_to_graph(copy_inst->getCopiedCollectionAsUse());
    }
  }
  // Handle LLVM instructions.
  else {
    if (auto *phi = dyn_cast<llvm::PHINode>(&I)) {
      // For each incoming operand to the PHI, add its use to the graph.
      for (auto &incoming : phi->incoming_values()) {
        this->add_use_to_graph(incoming);
      }
    }
  }

  return;
}

} // namespace llvm::memoir
