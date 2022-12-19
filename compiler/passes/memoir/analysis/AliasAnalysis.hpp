#ifndef COMMON_ALIASANALYSIS_H
#define COMMON_ALIASANALYSIS_H
#pragma once

#include <iostream>

#include "memoir/support/InternalDatatypes.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/analysis/AccessAnalysis.hpp"
#include "memoir/analysis/AllocationAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

/*
 * This file provides a siumple analysis interface to query information about
 * aliasing between MemOIR fields.
 *
 * Author(s): Tommy McMichen
 * Created: August 12, 2022
 */

namespace llvm::memoir {

// /*
//  * Alias Info
//  */
// enum AliasInfo { NONE, MAY, MUST };

// /*
//  * Alias Analysis
//  *
//  * This alias analysis provides alias information about MemOIR fields.
//  */
// class AliasAnalysis {
// public:
//   /*
//    * Returns true if this_field aliases with the other_field.
//    * This relationship is commutative.
//    */
//   static AliasInfo aliases(FieldSummary &this_field, FieldSummary
//   &other_field);
// }

} // namespace llvm::memoir

#endif
