#include "memoir/ir/Instructions.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace memoir {

// DeleteInst implementation
OPERAND(DeleteInst, Object, 0)
TO_STRING(DeleteInst, "delete")

} // namespace memoir
