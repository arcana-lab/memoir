#ifndef MEMOIR_SUPPORT_ASSOCLIST_H
#define MEMOIR_SUPPORT_ASSOCLIST_H

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

/**
 * An association list that maintain insertion order.
 */
template <typename K, typename V>
struct AssocList : public List<pair<K, V>> {

  using DataTy = List<pair<K, V>>;

  DataTy::iterator find(const K &key) {
    return std::find_if(this->begin(), this->end(), [&key](const auto &pair) {
      return pair.first == key;
    });
  }

  V &operator[](const K &key) {
    auto found = this->find(key);
    if (found != this->end()) {
      return found->second;
    }

    this->emplace_back(key, V());

    return this->back().second;
  }
};

} // namespace llvm::memoir

#endif // MEMOIR_SUPPORT_ASSOCLIST_H
