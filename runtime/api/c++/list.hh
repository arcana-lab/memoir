#include "sequence.hh"

namespace memoir {

template <typename T>
class list : sequence<T> {
public:
  using typename sequence<T>::value_type;
  using typename sequence<T>::size_type;
  using typename sequence<T>::difference_type;
  using typename sequence<T>::iterator;
  using typename sequence<T>::const_iterator;
  using typename sequence<T>::reverse_iterator;
  using typename sequence<T>::const_reverse_iterator;

  list() : sequence<T>((size_type)0) {}

  // Member functions.
  using sequence<T>::assign;

  // TODO
  // template <class InputIt>
  // void assign(InputIt first, InputIt last);

  // TODO
  // void assign(std::initializer_list<T> ilist);

  // Element access.
  using sequence<T>::front;
  using sequence<T>::back;

  // Iterators.
  using sequence<T>::begin;
  using sequence<T>::cbegin;
  using sequence<T>::end;
  using sequence<T>::cend;
  using sequence<T>::rbegin;
  using sequence<T>::crbegin;
  using sequence<T>::rend;
  using sequence<T>::crend;

  // Capacity.
  using sequence<T>::empty;
  using sequence<T>::size;

  // Modifiers.
  using sequence<T>::clear;
  using sequence<T>::insert;
  using sequence<T>::erase;

  void push_back(const T &value) {
    this->insert(value, this->size());
  }

  void push_back(T &&value) {
    this->insert(value, this->size());
  }

  void pop_back() {
    this->remove(this->size() - 1, this->size());
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

  // Operations.
  // TODO
  // void merge(list<T> &other);
  // void merge(list<T> &&other);
  // template <class Compare>
  // void merge(list<T> &other, Compare comp);
  // template <class Compare>
  // void merge(list<T> &&other, Compare comp);

  // TODO
  // void splice_after(const_iterator pos, list<T> &other);
  // void splice_after(const_iterator pos, list<T> &&other);
  // void splice_after(const_iterator pos,
  //                   list<T> &other,
  //                   const_iterator it);
  // void splice_after(const_iterator pos,
  //                   list<T> &&other,
  //                   const_iterator it);
  // void splice_after(const_iterator pos,
  //                   list<T> &other,
  //                   const_iterator first,
  //                   const_iterator last);
  // void splice_after(const_iterator pos,
  //                   list<T> &&other,
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
