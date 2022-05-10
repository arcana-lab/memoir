#pragma once

#include "noelle/core/Noelle.hpp"

#include "Utils.hpp"

/*
 * Pass to transform array of structs -> struct of arrays
 *
 * Author: Tommy McMichen
 * Created: April 12, 2022
 */

namespace struct_of_arrays {

class StructOfArrays {
private:
  Module &M;

  Noelle *noelle;

  std::unordered_set<CallInst *> callsToObjectIR;

public:
  StructOfArrays(Module &M, Noelle *noelle);

  void analyze();

  void transform();
};

} // namespace struct_of_arrays
