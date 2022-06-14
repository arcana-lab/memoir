#pragma once

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_set>

/*
 * Pass to normalize the object-ir runtime and object-ir programs.
 *
 * Author: Tommy McMichen
 * Created: June 13, 2022
 */

using namespace llvm;

namespace normalization {

class Normalization {
private:
  Module &M;

  std::unordered_set<CallInst *> callsToObjectIR;

  /*
   * Add the !objectir.internal metadata tag to the function
   */
  void setRuntimeMetadata(Function *F);

  const std::string OBJECTIR_INTERNAL = "objectir.internal";

public:
  Normalization(Module &M);

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

} // namespace normalization
