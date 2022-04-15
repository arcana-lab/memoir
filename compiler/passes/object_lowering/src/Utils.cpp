#include "Utils.hpp"

using namespace object_lowering;

bool isObjectIRCall(std::string const& functionName) {
  return FunctionNamesToObjectIR.find(functionName) != FunctionNamesToObjectIR.end();
}
