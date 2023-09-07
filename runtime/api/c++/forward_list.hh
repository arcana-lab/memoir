#include "sequence.hh"

namespace memoir {

template <typename T>
class forward_list : sequence<T> {
public:
  forward_list(std::size_t n) : sequence<T>(n) {}

  // Member functions.
  using sequence<T>::assign;

  // Element access.
  using sequence<T>::front;

  // Iterators.
  iterator before_begin() {
    return sequence<T>::iterator(this->_storage, -1);
  }

  const_iterator before_begin() const {
    return sequence<T>::iterator(this->_storage, -1);
  }

  using sequence<T>::begin;
  using sequence<T>::cbegin;
  using sequence<T>::end;
  using sequence<T>::cend;

  // Capacity.
  using sequence<T>::empty;

  // Modifiers.
  void clear() {
    MEMOIR_FUNC(sequence_remove)(this->_storage, 0, this->size());
  }

  iterator insert_after(const_iterator pos, const T &value) {
    this->insert(pos._index + 1, value);
    return iterator(this->_storage, pos._index + 1);
  }

  iterator insert_after(const_iterator pos, T &&value) {
    this->insert(pos._index + 1, value);
    return iterator(this->_storage, pos._index + 1);
  }

  // TODO
  // iterator insert_after(const_iterator pos, size_type count, const T &value);
  // template <class InputIt>
  // iterator insert_after(const_iterator pos, InputIt first, InputIt last);
  // iterator insert_after(const_iterator pos, std::initializer_list<T> ilist);

  iterator erase_after(const_iterator pos) {
    if (pos._storage != this->_storage) {
      return pos;
    }
    if (pos == this->end()) {
      return pos;
    }
    this->remove(pos._index + 1);
    return iterator(this->_storage, pos._index + 1);
  }

  iterator erase_after(const_iterator first, const_iterator last) {
    if (this->_storage != first.storage || this->_storage != last.storage) {
      return last;
    }
    this->remove(first._index + 1, last._index);
    return last;
  }

  void push_front(const T &value) {
    this->insert(value, 0);
  }

  void push_front(T &&value) {
    this->insert(value, 0);
  }

  void pop_front() {
    this->remove(0, 1);
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
  // void swap(forward_list<T> &other);

  // Operations.
  // TODO
  // void merge(forward_list<T> &other);
  // void merge(forward_list<T> &&other);
  // template <class Compare>
  // void merge(forward_list<T> &other, Compare comp);
  // template <class Compare>
  // void merge(forward_list<T> &&other, Compare comp);

  // TODO
  // void splice_after(const_iterator pos, forward_list<T> &other);
  // void splice_after(const_iterator pos, forward_list<T> &&other);
  // void splice_after(const_iterator pos,
  //                   forward_list<T> &other,
  //                   const_iterator it);
  // void splice_after(const_iterator pos,
  //                   forward_list<T> &&other,
  //                   const_iterator it);
  // void splice_after(const_iterator pos,
  //                   forward_list<T> &other,
  //                   const_iterator first,
  //                   const_iterator last);
  // void splice_after(const_iterator pos,
  //                   forward_list<T> &&other,
  //                   const_iterator first,
  //                   const_iterator last);

  // TODO
  // void remove(const T &value);
  // template <class UnaryPredicate>
  // void remove_if(UnaryPredicate p);

  // TODO
  // void reverse() noexcept;

  // TODO
  // void unique();
  // template <class BinaryPredicate>
  // void unique(BinaryPredicate p);

  // TODO
  // void sort();
  // template <class Compare>
  // void sort(Compare cmp);

protected:
};

} // namespace memoir
