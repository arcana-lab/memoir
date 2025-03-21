#ifndef MEMOIR_SUPPORT_WORKLIST_H
#define MEMOIR_SUPPORT_WORKLIST_H

#include "llvm/ADT/ArrayRef.h"

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

template <typename T,
          const bool VisitOnce = false,
          const bool PushFront = false,
          const bool PopFront = false>
struct WorkList {
protected:
  vector<T> _items;
  set<T> _present;

public:
  WorkList() : _items{}, _present{} {}

  bool present(const T &val) {
    return this->_present.count(val) > 0;
  }

  bool push(const T &val) {
    if (this->present(val)) {
      return false;
    }

    this->_present.insert(val);

    if constexpr (PushFront) {
      this->_items.push_front(val);
    } else {
      this->_items.push_back(val);
    }

    return true;
  }

  bool push(std::input_iterator auto begin, std::input_iterator auto end) {
    bool pushed = false;
    for (auto it = begin; it != end; ++it) {
      pushed |= this->push(*it);
    }
    return pushed;
  }

  const T &pop() {
    if constexpr (PopFront) {
      const auto &val = this->_items.front();
      this->_items.pop_front();

      if constexpr (not VisitOnce) {
        this->_present.erase(val);
      }

      return val;

    } else {
      const auto &val = this->_items.back();
      this->_items.pop_back();

      if constexpr (not VisitOnce) {
        this->_present.erase(val);
      }

      return val;
    }
  }

  bool empty() {
    return this->_items.empty();
  }
};

} // namespace llvm::memoir

#endif
