# Project
set(PassName "memoir_example")
project(${PassName})

# Sources
file(GLOB PassSrc "src/*.cpp")

get_property(memoir_sources GLOBAL PROPERTY MEMOIR_SOURCES)

# Declare the LLVM pass to compile
add_llvm_library(
  ${PassName}
  MODULE
  ${PassSrc}
  ${memoir_sources})
