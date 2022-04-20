#pragma once

#include "noelle/core/Noelle.hpp"

#include "Utils.hpp"

/*
 * Pass to perform object inlining optimization at object-IR level
 *
 * Author: Yian Su
 * Created: April 19, 2022
 */

namespace object_inlining {

class ObjectInlining {
private:
  Module &M;

  Noelle *noelle;

  std::unordered_set<CallInst *> callsToObjectIR;

public:
  ObjectInlining(Module &M, Noelle *noelle);
  
  void analyze();

  void transform();
};

} // namespace object_inlining
