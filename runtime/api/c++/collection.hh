#ifndef MEMOIRPP_COLLECTION_HH
#define MEMOIRPP_COLLECTION_HH
#pragma once

#include "memoir.h"

namespace memoir {

// Base element.
class element {
  // Nothing.
};

// Base collection.
class collection {
public:
  collection(memoir::Collection *storage) : _storage(storage) {}

protected:
  memoir::Collection *const _storage;
};

} // namespace memoir

#endif // MEMOIRPP_COLLECTION_HH
