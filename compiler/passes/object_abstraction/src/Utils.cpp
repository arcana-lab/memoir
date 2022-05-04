#include "Utils.hpp"

using namespace object_abstraction;

bool object_abstraction::isObjectIRCall(
    std::string functionName) {
  return FunctionNamesToObjectIR.count(functionName) != 0;
}
