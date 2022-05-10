#include "Utils.hpp"

bool isObjectIRCall(std::string functionName) {
  return FunctionNamesToObjectIR.find(functionName)
         != FunctionNamesToObjectIR.end();
}

ObjectIRFunc getObjectIRCall(std::string functionName) {
  auto IT = FunctionNamesToObjectIR.find(functionName);
  if (IT == FunctionNamesToObjectIR.end()) {
    return ObjectIRFunc::NONE;
  }

  return (*IT).second;
}
