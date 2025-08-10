#ifndef MEMOIR_SUPPORT_UNIONFIND_H
#define MEMOIR_SUPPORT_UNIONFIND_H

#include <cstdint>
#include <iterator>

#include "llvm/Support/raw_ostream.h"

#include "memoir/support/DataTypes.hpp"

namespace llvm::memoir {

template <typename T>
struct UnionFind {
protected:
  using ParentMap = Map<T, T>;
  using Iter = typename ParentMap::iterator;
  using Size = size_t;
  using SizeMap = Map<T, Size>;

  ParentMap _parent;
  SizeMap _size;

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
    if (this->size(u) > this->size(t)) {
      std::swap(t, u);
    }

    // Merge u into t
    this->parent(u) = t;
    this->size(t) += this->size(u);

    // Return
    return t;
  }

  Iter begin() {
    this->reify();
    return this->_parent.begin();
  }
  Iter end() {
    return this->_parent.end();
  }

  Size size() const {
    return this->_parent.size();
  }

  Size &size(const T &t) {
    return this->_size.at(t);
  }

  T &parent(const T &t) {
    return this->_parent.at(t);
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
};

} // namespace llvm::memoir

#endif // MEMOIR_SUPPORT_UNIONFIND_H
