#pragma once

#include "noelle/core/Noelle.hpp"

#include "Utils.hpp"

/*
 * Pass to perform lowering from object-ir to LLVM IR
 *
 * Author: Tommy McMichen
 * Created: March 29, 2022
 */

namespace object_lowering {

class ObjectLowering {
private:
  Module &M;

  Noelle *noelle;

  std::unordered_set<CallInst *> callsToObjectIR;

public:
  ObjectLowering(Module &M, Noelle *noelle);

  void analyze();

  void transform();
};

} // namespace object_lowering
