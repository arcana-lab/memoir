#include "sequence.hh"

namespace memoir {

template <typename T>
class vector : public sequence<T> {
public:
  vector(std::size_t n) : sequence<T>(n) {}

  // Member functions.
  void assign(std::size_t count, const T &value) {
    if (this->size() != count) {
      MEMOIR_FUNC(delete_collection)(this->_storage);
      this->_storage =
          MEMOIR_FUNC(allocate_sequence)(to_memoir_type<T>(), count);
    }
    for (std::size_t i = 0; i < count; ++i) {
      this[i] = T;
    }
  }

  // TODO
  // template <class InputIt>
  // void assign(InputIt first, InputIt last);

  // TODO
  // void assign(std::initializer_list<T> ilist);

  // Element access.
  // None.

  // Iterators.
  // None.

  // Capacity.
  void reserve(std::size_t new_capacity) {
    // TODO: give a hint to the compiler.
  }

  void shrink_to_fit() {
    // TODO: give a hint to the compiler.
  }

  // Modifiers.
  void clear() {
    MEMOIR_FUNC(sequence_remove)(this->_storage, 0, this->size());
  }

  // TODO
  // template <class Args...>
  // iterator emplace(iterator pos, Args &&... args) {}

  iterator erase(iterator pos) {
    if (pos._storage != this->_storage) {
      return pos;
    }
    if (pos == this->end()) {
      return pos;
    }
    this->remove(pos._index);
    return iterator(this->_storage, pos._index);
  }

  iterator erase(iterator first, iterator last) {
    if (this->_storage != first.storage || this->_storage != last.storage) {
      return last;
    }
    if (first == last) {
      return last;
    }
    if (last == this->end()) {
      this->remove(first._index, last._index);
      return last;
    } else {
      this->remove(first._index, last._index);
      return iterator(this->_storage, first._index);
    }
  }

  void push_back(const T &value) {
    this->insert(value, this->size());
  }

  void push_back(T &&value) {
    this->insert(value, this->size());
  }

  void pop_back() {
    this->remove(this->size() - 1, this->size());
  }

  void resize(size_type count) {
    if (count == this->size()) {
      return;
    }
    this->remove(count, this->size());
  }

  void resize(size_type count, const T &value) {
    this->assign(count, value);
  }

  // TODO
  // void swap(vector<T> &other);

protected:
};

} // namespace memoir
