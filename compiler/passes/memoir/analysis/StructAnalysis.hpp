#ifndef COMMON_STRUCTANALYSIS_H
#define COMMON_STRUCTANALYSIS_H
#pragma once

#include "memoir/utility/Assert.hpp"
#include "memoir/utility/InternalDataTypes.hpp"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Structs.hpp"

namespace llvm::memoir {

class StructAnalysis {
public:
  static StructAnalysis &get();

  static Struct *analyze(llvm::Value &V);

  static void invalidate();

  /*
   * Analysis
   */
  Struct *getStruct(llvm::Value &V);

  /*
   * This class is not clonable nor assignable.
   */
  StructAnalysis(StructAnalysis &other) = delete;
  void operator=(const StructAnalysis &other) = delete;

protected:
  /*
   * Private constructor and logistics
   */
  StructAnalysis();

  void _invalidate();
};

} // namespace llvm::memoir

#endif
