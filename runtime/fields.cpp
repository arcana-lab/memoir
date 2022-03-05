#include "objects.hpp"

using namespace objectir;

ScalarField::ScalarField(T init) {
  // Construct a scalar field
  this->value = init;
};

PointerField::PointerField(T *obj) {
  // Construct a pointer field
  this->obj = obj;
};
