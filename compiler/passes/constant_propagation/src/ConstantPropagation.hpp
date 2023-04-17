#ifndef CONSTPROP_H
#define CONSTPROP_H
#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "noelle/core/Noelle.hpp"

#include "memoir/utility/Metadata.hpp"

#include <unordered_set>

/*
 * Pass to perform constant propagation on MemOIR collections.
 *
 * Author: Nick Wanninger
 * Created: April 17, 2023
 */

using namespace llvm;

namespace constprop {

class ConstantPropagation {
private:
  Module &M;
  Noelle &noelle;

public:
  ConstantPropagation(Module &M, Noelle &noelle);

  /*
   * Analyze the program
   */
  void analyze();

  /*
   * Transform the program
   */
  void transform();
};

} // namespace constprop

#endif
