#include "Utils.hpp"

using namespace object_lowering;

bool object_lowering::isObjectIRCall(std::string functionName) {
  return FunctionNamesToObjectIR.find(functionName) != FunctionNamesToObjectIR.end();
}
