#include "llvm/Support/CommandLine.h"

#include "memoir/support/Print.hpp"

using namespace llvm::memoir;

// Instantiate the command line options we need.
Verbosity llvm::memoir::VerboseLevel;
static llvm::cl::opt<llvm::memoir::Verbosity, true> VerboseLevelOpt(
    "memoir-verbose",
    llvm::cl::desc("Set the verbosity"),
    llvm::cl::values(
        clEnumValN(noverbosity, "none", "disable verbose messages"),
        clEnumVal(quick, "only enable short-form messages"),
        clEnumVal(detailed, "enable all verbose messages")),
    llvm::cl::location(llvm::memoir::VerboseLevel));
