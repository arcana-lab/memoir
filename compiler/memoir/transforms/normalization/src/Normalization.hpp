#ifndef NORMALIZATION_H
#define NORMALIZATION_H
#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/utility/Metadata.hpp"

/*
 * Pass to normalize the MEMOIR runtime and MEMOIR programs.
 *
 * Author: Tommy McMichen
 * Created: June 13, 2022
 */

namespace llvm::memoir {

class Normalization {
private:
  llvm::Module &M;

public:
  Normalization(llvm::Module &M);

  /*
   * Analyze the program
   */
  void analyze();

  /*
   * Transform the program
   */
  void transform();

  /*
   * Transform the runtime bitcode
   */
  void transformRuntime();
};

} // namespace llvm::memoir

#endif
