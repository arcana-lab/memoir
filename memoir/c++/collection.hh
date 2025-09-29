#ifndef MEMOIRPP_COLLECTION_HH
#define MEMOIRPP_COLLECTION_HH
#pragma once

#include "memoir.h"

namespace memoir {

#define always_inline __attribute__((always_inline))

// Base element.
class element {
  // Nothing.
};

// Base collection.
class collection {
public:
  always_inline collection(memoir::Collection *storage) : _storage(storage) {}

  always_inline void clear() {
    MUT_FUNC(clear)(this->_storage);
  }

protected:
  collection_ref _storage;
};

} // namespace memoir

#endif // MEMOIRPP_COLLECTION_HH
