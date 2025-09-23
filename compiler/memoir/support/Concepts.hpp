#ifndef MEMOIR_SUPPORT_CONCEPTS_H
#define MEMOIR_SUPPORT_CONCEPTS_H

#include <type_traits>

namespace memoir {

template <class T, class U>
concept Derived = std::is_base_of<U, T>::value;

template <class T, class U>
concept Same = std::is_same<U, T>::value;

template <class T, class U>
concept NotSame = not std::is_same<U, T>::value;

} // namespace memoir

#endif
