#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *aTy = nameObjectType("A",
                           2,
                           getUInt64Type(),
                           getNamedType("B"));

Type *bTy = nameObjectType("B",
                           2,
                           getNamedType("A"),
                           getNamedType("B"));

int main() {
  std::cerr << aTy->toString() << "\n";
  std::cerr << bTy->toString() << "\n";

  Object *myA = buildObject(aTy);
  Object *myB = buildObject(bTy);
}
