#ifndef MEMOIR_SUPPORT_GRAPH_H
#define MEMOIR_SUPPORT_GRAPH_H

#include "memoir/support/graph_lite.h"

namespace llvm::memoir {

using namespace graph_lite;

template <typename NodeType, typename EdgeType, typename NodePropType = void>
class DirectedGraph : public graph_lite::Graph<NodeType,
                                               NodePropType,
                                               EdgeType,
                                               EdgeDirection::DIRECTED,
                                               MultiEdge::DISALLOWED,
                                               SelfLoop::ALLOWED,
                                               graph_lite::Map::MAP,
                                               Container::UNORDERED_SET,
                                               Logging::DISALLOWED> {
public:
  template <typename T>
  struct iter_wrapper {
    T _begin, _end;

    iter_wrapper(T begin, T end) : _begin(begin), _end(end) {}

    T begin() {
      return _begin;
    }
    T end() {
      return _end;
    }
  };

  auto outgoing(const NodeType &n) {
    auto [begin, end] = this->out_neighbors(n);
    return iter_wrapper<decltype(begin)>(begin, end);
  }

  auto incoming(const NodeType &n) {
    auto [begin, end] = this->in_neighbors(n);
    return iter_wrapper<decltype(begin)>(begin, end);
  }

  EdgeType get_edge(const NodeType &a, const NodeType &b) {
    for (auto &[n, e] : this->outgoing(a))
      if (n == b)
        return e.prop();
    MEMOIR_UNREACHABLE("Unable to find edge!");
  }

  // Given a start node, produce a vector that is the depth first
  // traversal starting from that node.
  Vector<NodeType> depth_first_order(NodeType start) {
    // procedure DFS_iterative(G, v) is
    // let S be a stack
    // S.push(v)
    // while S is not empty do
    //     v = S.pop()
    //     if v is not labeled as discovered then
    //         label v as discovered
    //         for all edges from v to w in G.adjacentEdges(v) do
    //             S.push(w)

    Vector<NodeType> out;
    Set<NodeType> discovered;
    Stack<NodeType> s;
    s.push(start);
    while (not s.empty()) {
      auto v = s.top();
      s.pop();

      // If v was not labeled as discovered
      if (discovered.find(v) == discovered.end()) {
        // label v as discovered
        discovered.insert(v);
        // add it to the list
        out.push_back(v);

        for (auto &[w, _] : this->outgoing(v)) {
          s.push(w);
        }
      }
    }
    return out;
  }

  Vector<NodeType> breadth_first_order(NodeType start) {
    Vector<NodeType> out;
    Set<NodeType> explored;
    Queue<NodeType> Q;
    Q.push(start);
    explored.insert(start);

    while (not Q.empty()) {
      auto v = Q.front();
      Q.pop();
      out.push_back(v);

      for (auto &[w, _] : this->outgoing(v)) {
        if (explored.find(w) == explored.end()) {
          explored.insert(w);
          Q.push(w);
        }
      }
    }

    return out;
  }

  bool topological_order_helper(NodeType node,
                                Vector<NodeType> &order,
                                Set<NodeType> &permanent_mark,
                                Set<NodeType> &temporary_mark) {
    if (permanent_mark.count(node) != 0) {
      return true;
    }

    if (temporary_mark.count(node) != 0) {
      // We detected a cycle! Signal that the ordering has failed.
      return false;
    }

    // Temporarily mark the node.
    temporary_mark.insert(node);

    // For each outgoing edge:
    for (auto [out, _] : this->outgoing(node)) {
      if (!this->topological_order_helper(out,
                                          order,
                                          permanent_mark,
                                          temporary_mark)) {
        return false;
      }
    }

    // Remove temporary mark from node.
    temporary_mark.erase(node);

    // Permanently mark the node.
    permanent_mark.insert(node);

    // Prepend the node to the order.
    order.push_back(node);

    return true;
  }

  // Implementation of DFS topological sorting.
  Option<Vector<NodeType>> topological_order() {
    Vector<NodeType> order = {};
    order.reserve(this->size());

    Set<NodeType> permanent_mark = {};
    Set<NodeType> temporary_mark = {};

    // While there exists a node without a permanent mark:
    for (auto node_it = this->begin(); node_it != this->end(); ++node_it) {
      auto node = *node_it;

      // Select an unmarked node.
      if (permanent_mark.count(node) == 0) {
        if (!topological_order_helper(node,
                                      order,
                                      permanent_mark,
                                      temporary_mark)) {
          return {};
        }
      }
    }

    std::reverse(order.begin(), order.end());

    return order;
  }

  // Connected components.
  struct ConnectedComponent : public Set<NodeType> {};
  struct Components : public Map<NodeType, ConnectedComponent> {};

  void tarjan_scc_helper(NodeType n,
                         Components &components,
                         uint32_t &index,
                         Map<NodeType, uint32_t> &indices,
                         Map<NodeType, uint32_t> &lowlink,
                         Vector<NodeType> &stack) {
    indices[n] = index;
    lowlink[n] = index;
    ++index;
    stack.push_back(n);

    // For each outgoing edge of n->m:
    for (auto [to_node, _] : this->outgoing(n)) {
      // If m has no index, recurse on it.
      if (indices.count(to_node) == 0) {
        tarjan_scc_helper(to_node, components, index, indices, lowlink, stack);
        lowlink[n] = std::min(lowlink[n], lowlink[to_node]);
      }

      // Otherwise, if m is on the stack, then n and m are in the same SCC,
      // ignore this edge.
      else if (std::find(stack.begin(), stack.end(), to_node) != stack.end()) {
        lowlink[n] = std::min(lowlink[n], indices[to_node]);
      }
    }

    // If we have a root node, pop the stack and generate an SCC.
    if (lowlink[n] == indices[n]) {
      ConnectedComponent &scc = components[n];

      NodeType pop_node;
      do {
        pop_node = stack.back();
        stack.pop_back();
        scc.insert(pop_node);
      } while (pop_node != n);
    }

    return;
  }

  Components tarjan_scc() {
    // Initialze an empty set of components.
    Components components = {};

    // Initialize bookkeeping.
    uint32_t index = 0;
    Map<NodeType, uint32_t> indices = {};
    Map<NodeType, uint32_t> lowlink = {};
    Vector<NodeType> stack = {};

    // For each node, if its index is undefined, analyze it.
    for (auto node_it = this->begin(); node_it != this->end(); ++node_it) {
      auto node = *node_it;
      if (indices.count(node) == 0) {
        tarjan_scc_helper(node, components, index, indices, lowlink, stack);
      }
    }

    // Return the components.
    return components;
  }

  // Filtered view of graph.
  struct FilteredView {
  public:
    FilteredView(DirectedGraph<NodeType, EdgeType, NodePropType> &graph,
                 const Set<NodeType> &filter)
      : _graph(graph),
        _filter(filter) {}

    template <typename T>
    struct filtered_iter_wrapper {
      T _begin, _end;
      const Set<NodeType> &_filter;

      filtered_iter_wrapper(T begin, T end, const Set<NodeType> &filter)
        : _begin(begin),
          _end(end),
          _filter(filter) {}

      filtered_iter_wrapper &operator++(filtered_iter_wrapper &it) {
        while (++it != _end) {
          if (_filter.count(*it) != 0) {
            return it;
          }
        }
      }

      T begin() {
        return _begin;
      }

      T end() {
        return _end;
      }
    };

    auto begin() {
      return this->_filter.begin();
    }

    auto end() {
      return this->_filter.end();
    }

    auto size() {
      return this->_filter.size();
    }

    auto find(const NodeType &n) {
      return this->_filter.find(n);
    }

    auto count(const NodeType &n) {
      return this->_filter.count(n);
    }

    auto outgoing(const NodeType &n) {
      auto [begin, end] = this->out_neighbors(n);
      return filtered_iter_wrapper<decltype(begin)>(begin, end, *this);
    }

    auto incoming(const NodeType &n) {
      auto [begin, end] = this->in_neighbors(n);
      return filtered_iter_wrapper<decltype(begin)>(begin, end, *this);
    }

    operator DirectedGraph<NodeType, EdgeType, NodePropType>() {
      return this->_graph;
    }

  protected:
    DirectedGraph<NodeType, EdgeType, NodePropType> &_graph;
    Set<NodeType> _filter;
  };

  // Condensation graph (SCCDAG).
  struct Condensation
    : public DirectedGraph<
          NodeType,
          EdgeType,
          DirectedGraph<NodeType, EdgeType, NodePropType>::FilteredView> {};

  // Compute the condensation of the graph.
  Condensation condense() {
    Condensation condensation;

    // Get the strongly connected components of the graph.
    auto components = this->tarjan_scc();

    debugln("Strongly-connected Components: ");
    for (auto const &[root, nodes] : components) {
      debugln(" ├─┬─╸ ", *root);
      for (auto *node : nodes) {
        if (node != root) {
          debugln(" │ ├─╸ ", *node);
        }
      }
      debugln(" │ ╹ ");
    }
    debugln(" ╹");

    // Create the condensation of each component.
    for (auto const &[root, nodes] : components) {
      // Create the node with subgraph view containing the component nodes.
      condensation.add_node_with_prop(root, *this, nodes);

      // Add edges not contained in the component to the condensation graph.
      for (auto [in, _] : this->incoming(root)) {
        if (nodes.count(in) == 0) {
          condensation.add_edge_with_prop(in, root, this->get_edge(in, root));
        }
      }

      for (auto [out, _] : this->outgoing(root)) {
        if (nodes.count(out) == 0) {
          condensation.add_edge_with_prop(root, out, this->get_edge(root, out));
        }
      }
    }

    // Return the condensation graph.
    return condensation;
  }
};

} // namespace llvm::memoir

#endif // MEMOIR_SUPPORT_GRAPH_H
