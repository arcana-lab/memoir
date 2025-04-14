#ifndef MEMOIR_SUPPORT_UNIONFIND_H
#define MEMOIR_SUPPORT_UNIONFIND_H

#include <cstdint>
#include <iterator>

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

template <typename T>
struct UnionFind {
public:
  void insert(T t) {
    auto found = this->_parent.find(t);
    if (found == this->_parent.end()) {
      this->_parent[t] = t;
      this->_size[t] = 1;
      return;
    }

    return;
  }

  T find(T t) {
    // If t has no parent, insert it.
    auto found = this->_parent.find(t);
    if (found == this->_parent.end()) {
      this->insert(t);
      return t;
    }

    // If t is its own parent, return it.
    if (found->second == t) {
      return t;
    }

    // Otherwise, recurse.
    auto new_t = this->find(found->second);

    // Update the parent of t to the result of find.
    found->second = new_t;

    // Return the result of find.
    return new_t;
  }

  T merge(T t, T u) {
    // Find t and u
    t = this->find(t);
    u = this->find(u);

    // If u.size > t.size, swap them
    if (this->_size[u] > this->_size[t]) {
      std::swap(t, u);
    }

    // Merge u into t
    this->_parent[u] = t;
    this->_size[t] += this->_size[u];

    // Return
    return t;
  }

  typename Map<T, T>::iterator begin() {
    this->reify();
    return this->_parent.begin();
  }
  typename Map<T, T>::iterator end() {
    return this->_parent.end();
  }

  size_t size() const {
    return this->_parent.size();
  }

  /**
   * Iterate over all members of the union-find data structure, finding its real
   * parent and updating it in the data structure.
   */
  void reify() {
    for (auto &[item, parent] : this->_parent) {
      this->find(item);
    }
    return;
  }

protected:
  Map<T, T> _parent;
  Map<T, size_t> _size;
};

} // namespace llvm::memoir

#endif // MEMOIR_SUPPORT_UNIONFIND_H
