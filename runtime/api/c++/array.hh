#include "sequence.hh"

namespace memoir {

template <typename T, std::size_t N>
class array : public sequence<T> {
public:
  array() : sequence<T>(N) {}

  // Invalid operations.
  void insert(T, std::size_t) = delete;
  void insert(const sequence<T> &, std::size_t) = delete;
  void remove(std::size_t, std::size_t) = delete;
  void remove(std::size_t) = delete;
  void append(std::size_t) = delete;
  void split(std::size_t, std::size_t) = delete;

protected:
}; // class array

} // namespace memoir
