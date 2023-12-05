# Pass
set(PassName "memoir_example")

# Sources
file(GLOB PassSrc "src/*.cpp")

# Declare the LLVM pass to compile
add_llvm_library(
  ${PassName}
  MODULE
  ${PassSrc}
  ${memoir_sources})
