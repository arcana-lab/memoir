# Project
set(PassName "ConstantPropagation")
project(${PassName})

# Dependences
#include(${CMAKE_CURRENT_SOURCE_DIR}/../../../src/scripts/DependencesCMake.txt)

# Sources
file(GLOB PassSrc "src/*.cpp")

get_property(memoir_sources GLOBAL PROPERTY MEMOIR_SOURCES)

# Declare the LLVM pass to compile
add_llvm_library(
  ${PassName}
  MODULE
  ${PassSrc}
  ${memoir_sources})

install(TARGETS ${PassName}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
