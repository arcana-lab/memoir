#include <iostream>

#include "types.h"

namespace objectir {

/*
 * Type Factory singleton implementation
 */
TypeFactory *TypeFactory::getInstance() {
  static TypeFactory *instance = nullptr;

  if (!instance) {
    instance = new TypeFactory();
  }

  return instance;
}

TypeFactory::TypeFactory() {
  // Do nothing.
}

void TypeFactory::registerType(std::string name,
                               Type *type) {
  if (nameToType.find(name) != this->nameToType.end()) {
    // Resolve the stub type
    auto findType = nameToType.at(name);
    if (findType->getCode() == TypeCode::StubTy) {
      auto stubType = (StubType *)findType;
      stubType->resolvedType = type;
      nameToType[name] = type;
    } else {
      std::cerr
          << "ERROR: type " << name
          << " is already registered by another type\n";
      exit(1);
    }
  } else {
    // Register the given type
    nameToType[name] = type;
  }
}

Type *TypeFactory::getType(std::string name) {
  if (nameToType.find(name) != nameToType.end()) {
    // Fetch the named type
    return nameToType.at(name);
  } else {
    // Create a stub type if the type isn't registered yet
    auto stubType = new StubType(name);
    nameToType[name] = stubType;
    return stubType;
  }
}

} // namespace objectir
