#ifndef MEMOIR_BACKEND_STLVECTOR_H
#define MEMOIR_BACKEND_STLVECTOR_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <type_traits>

#include <vector>

#include <boost/dynamic_bitset.hpp>

template <typename Elem>
struct Vector;

template <typename Elem>
struct Vector : std::vector<Elem> {
  Vector() : Vector(0) {}
  Vector(size_t sz) : std::vector<Elem>(sz) {}
  Vector(std::input_iterator auto begin, std::input_iterator auto end)
    : std::vector<Elem>(begin, end) {}
  ~Vector() {
    // TODO: if the element is a collection pointer, delete it too
  }

  Elem *get(size_t i) {
    return &*std::next(this->begin(), i);
  }

  Elem read(size_t i) {
    return (*this)[i];
  }

  void write(size_t i, Elem v) {
    (*this)[i] = v;
  }

  void remove(size_t i) {
    this->erase(std::next(this->begin(), i));
  }

  void remove(size_t begin, size_t end) {
    this->erase(std::next(this->begin(), begin), std::next(this->begin(), end));
  }

  using std::vector<Elem>::clear;

  using std::vector<Elem>::insert;

  void insert(size_t i) {
    this->insert(std::next(this->begin(), i), Elem());
  }

  void insert(size_t i, Elem v) {
    this->insert(std::next(this->begin(), i), v);
  }

  void insert(size_t i, Vector<Elem> *other) {
    this->insert(std::next(this->begin(), i), other->cbegin(), other->cend());
  }

  void insert(size_t i, Vector<Elem> *other, size_t begin, size_t end) {
    this->insert(std::next(this->begin(), i),
                 std::next(other->cbegin(), begin),
                 std::next(other->cbegin(), end));
  }

  using std::vector<Elem>::size;

  Vector<Elem> *copy() {
    return new Vector<Elem>(this->cbegin(), this->cend());
  }

  Vector<Elem> *copy(size_t begin, size_t end) {
    return new Vector<Elem>(std::next(this->cbegin(), begin),
                            std::next(this->cbegin(), end));
  }

  struct iterator {
    size_t _idx;
    as_primitive_t<Elem> _val;
    std::vector<Elem>::iterator _it;
    std::vector<Elem>::iterator _ie;

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }

      ++this->_idx;
      this->_val = into_primitive(*this->_it);
      this->_it = std::next(this->_it);

      return true;
    }
  };

  using std::vector<Elem>::begin;
  using std::vector<Elem>::end;

  void begin(iterator *iter) {
    iter->_idx = -1;
    iter->_it = this->begin();
    iter->_ie = this->end();
  }

  struct reverse_iterator {
    size_t _idx;
    as_primitive_t<Elem> _val;
    std::vector<Elem>::reverse_iterator _it;
    std::vector<Elem>::reverse_iterator _ie;

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }

      --this->_idx;
      this->_val = into_primitive(*this->_it);
      this->_it = std::next(this->_it);

      return true;
    }
  };

  using std::vector<Elem>::rbegin;
  using std::vector<Elem>::rend;

  void rbegin(reverse_iterator *iter) {
    iter->_idx = this->size();
    iter->_it = this->rbegin();
    iter->_ie = this->rend();
  }
};

template <>
struct Vector<bool> : boost::dynamic_bitset<> {
  Vector(size_t sz) : boost::dynamic_bitset<>(sz) {}
  Vector(std::input_iterator auto begin, std::input_iterator auto end)
    : boost::dynamic_bitset<>(begin, end) {}
  ~Vector() {
    // TODO: if the element is a collection pointer, delete it too
  }

  bool read(size_t i) {
    return this->test(i);
  }

  void write(size_t i, bool v) {
    this->set(i, v);
  }

  using boost::dynamic_bitset<>::clear;

#if 0 // TODO
  void remove(size_t i) {
    this->erase(std::next(this->begin(), i));
  }

  void remove(size_t begin, size_t end) {
    this->erase(std::next(this->begin(), begin), std::next(this->begin(), end));
  }

  using boost::dynamic_bitset<>::insert;

  void insert(size_t i) {
    this->insert(std::next(this->begin(), i), bool());
  }

  void insert(size_t i, bool v) {
    this->insert(std::next(this->begin(), i), v);
  }

  void insert(size_t i, Vector<bool> *other) {
    this->insert(std::next(this->begin(), i), other->cbegin(), other->cend());
  }

  void insert(size_t i, Vector<bool> *other, size_t begin, size_t end) {
    this->insert(std::next(this->begin(), i),
                 std::next(other->cbegin(), begin),
                 std::next(other->cbegin(), end));
  }


  Vector<bool> *copy() {
    return new Vector<bool>(this->cbegin(), this->cend());
  }

  Vector<bool> *copy(size_t begin, size_t end) {
    return new Vector<bool>(std::next(this->cbegin(), begin),
                            std::next(this->cbegin(), end));
  }

#endif

  size_t size() {
    return this->count();
  }

  struct iterator {
    size_t _idx;
    bool _val;
    Vector<bool> *_vec;

    bool next() {
      if (this->_idx == this->_vec->size()) {
        return false;
      }

      ++this->_idx;
      this->_val = this->_vec->test(this->_idx);

      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_idx = -1;
    iter->_vec = this;
  }

  struct reverse_iterator {
    size_t _idx;
    bool _val;
    Vector<bool> *_vec;

    bool next() {
      if (this->_idx == 0) {
        return false;
      }

      --this->_idx;
      this->_vec->test(this->_idx);

      return true;
    }
  };

  void rbegin(reverse_iterator *iter) {
    iter->_idx = this->size();
    iter->_vec = this;
  }
};

#endif // MEMOIR_BACKEND_STLVECTOR_H
