#ifndef COMMON_INTERNALDATATYPES_H
#define COMMON_INTERNALDATATYPES_H
#pragma once

/*
 * This file contains a internal data types used by MemOIR internal passes.
 * It provides a wrapper around some stdlib objects to make it easier to
 *   swap between debug and release versions of the codebase.
 *
 * Author(s): Tommy McMichen
 * Created: July 15, 2022
 */

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <list>
#include <queue>
#include <stack>
#include <tuple>
#include <vector>

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>

// LLVM Data types
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm::memoir {

/*
 * We use references in all places where possible.
 */
template <typename T>
using opt = std::optional<T>;

template <typename T>
using ref = std::reference_wrapper<T>;

template <typename T>
using opt_ref = std::optional<std::reference_wrapper<T>>;

template <typename T>
using unique = std::unique_ptr<T>;

template <typename T>
using shared = std::shared_ptr<T>;

template <typename T>
using weak = std::weak_ptr<T>;

/*
 * Some utility types that let us unwrap the inner type from a
 * reference_wrapper.
 */
template <typename T>
struct unwrap_ref {
  using type = T;
};

template <typename T>
struct unwrap_ref<std::reference_wrapper<T>> {
  using type = T &;
};

template <typename T>
using unwrap_ref_type = typename unwrap_ref<T>::type;

/*
 * Define the internal types used for debug vs release versions
 *
 * We want to use the ordered versions of collections when in debug mode
 *   otherwise, for release, we want to use the unordered versions as
 *   they are faster.
 */

#if DEBUG
template <typename T, typename U>
using map = std::map<T, U, std::less<unwrap_ref_type<T>>>;
#else
template <typename T, typename U>
using map = std::unordered_map<T, U, std::hash<unwrap_ref_type<T>>>;
#endif

template <typename T, typename U>
using ordered_map = std::map<T, U, std::less<unwrap_ref_type<T>>>;

#if DEBUG
template <typename T>
using set = std::set<T, std::less<unwrap_ref_type<T>>>;
#else
template <typename T>
using set = std::unordered_set<T, std::hash<unwrap_ref_type<T>>>;
#endif

template <typename T>
using ordered_set = std::set<T, std::less<unwrap_ref_type<T>>>;

#if DEBUG
template <typename T, typename U>
using multimap = std::multimap<T, U>;
#else
template <typename T, typename U>
using multimap = std::unordered_multimap<T, U>;
#endif

template <typename T, typename U>
using ordered_multimap = std::multimap<T, U>;

template <typename T, typename TT, typename U, typename UU>
inline typename std::multimap<T, U>::iterator insert_unique(
    std::multimap<T, U> &mmap,
    const TT &key,
    const UU &value) {
  auto range = mmap.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second == value) {
      return it;
    }
  }
  return mmap.insert(std::make_pair(key, value));
}

template <typename T, typename TT, typename U, typename UU>
inline typename std::unordered_multimap<T, U>::iterator insert_unique(
    std::unordered_multimap<T, U> &mmap,
    const TT &key,
    const UU &value) {
  auto range = mmap.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second == value) {
      return it;
    }
  }
  return mmap.insert(std::make_pair(key, value));
}

#if DEBUG
template <typename T>
using multiset = std::multiset<T, std::less<unwrap_ref_type<T>>>;
#else
template <typename T>
using multiset = std::unordered_multiset<T, std::hash<unwrap_ref_type<T>>>;
#endif

template <typename T>
using ordered_multiset = std::multiset<T, std::less<unwrap_ref_type<T>>>;

template <typename T>
using vector = std::vector<T>;

template <typename T>
using list = std::list<T>;

template <typename T>
using stack = std::stack<T>;

template <typename T>
using queue = std::queue<T>;

// Pair.
template <typename T1, typename T2>
using pair = std::pair<T1, T2>;

template <typename T1, typename T2>
inline pair<T1, T2> make_pair(T1 first, T2 second) {
  return std::make_pair(first, second);
}

// Tuple
template <typename... Ts>
using tuple = std::tuple<Ts...>;

template <typename... Ts>
inline tuple<Ts...> make_tuple(Ts... args) {
  return std::make_tuple<Ts...>(std::forward<Ts...>(args)...);
}

} // namespace llvm::memoir

#endif
