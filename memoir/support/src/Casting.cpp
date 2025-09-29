#include "memoir/support/Casting.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Keywords.hpp"
#include "memoir/support/Concepts.hpp"

namespace memoir {

namespace detail {

MemOIRInst *get_memoir(llvm::Instruction &inst) {
  return MemOIRInst::get(inst);
}

} // namespace detail

} // namespace memoir
