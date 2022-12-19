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

#if DEBUG
#  include <map>
#  include <set>
#else
#  include <unordered_map>
#  include <unordered_set>
#endif

#include <queue>
#include <stack>
#include <vector>

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>

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

#if DEBUG
template <typename T>
using set = std::set<T, std::less<unwrap_ref_type<T>>>;
#else
template <typename T>
using set = std::unordered_set<T, std::hash<unwrap_ref_type<T>>>;
#endif

template <typename T>
using vector = std::vector<T>;

template <typename T>
using stack = std::stack<T>;

template <typename T>
using queue = std::queue<T>;

template <typename T, std::size_t Extent = std::dynamic_extent>
using span = std::span<T, Extent>;

} // namespace llvm::memoir

#endif
