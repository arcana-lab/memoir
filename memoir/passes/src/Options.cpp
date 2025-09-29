#include "llvm/Support/CommandLine.h"

#include "memoir/support/Print.hpp"

using namespace memoir;

// Instantiate the command line options we need.
Verbosity memoir::VerboseLevel;
static llvm::cl::opt<memoir::Verbosity, true> VerboseLevelOpt(
    "memoir-verbose",
    llvm::cl::desc("Set the verbosity"),
    llvm::cl::values(
        clEnumValN(noverbosity, "none", "disable verbose messages"),
        clEnumVal(quick, "only enable short-form messages"),
        clEnumVal(detailed, "enable all verbose messages")),
    llvm::cl::location(memoir::VerboseLevel));
