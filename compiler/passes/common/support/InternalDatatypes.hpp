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

namespace llvm::memoir {

/*
 * Define the internal types used for debug vs release versions
 *
 * We want to use the ordered versions of collections when in debug mode
 *   otherwise, for release, we want to use the unordered versions as
 *   they are faster.
 */
template <typename T, typename U>
#if DEBUG
using map = std::map<T, U>;
#else
using map = std::unordered_map<T, U>;
#endif

template <typename T>
#if DEBUG
using set = std::set<T>;
#else
using set = std::unordered_set<T>;
#endif

} // namespace llvm::memoir

#endif
