# Project
set(pass_name "CollectionLowering")
project(${pass_name})

# Set the pass name so C/C++ files can see it.
add_definitions( -DMEMOIR_PASS_NAME=${pass_name})

# Dependences
#include(${CMAKE_CURRENT_SOURCE_DIR}/../../../src/scripts/DependencesCMake.txt)

# Sources
file(GLOB pass_sources "src/*.cpp")

get_property(memoir_sources GLOBAL PROPERTY MEMOIR_SOURCES)

# Declare the LLVM pass to compile
add_llvm_library(
  ${pass_name}
  MODULE
  ${pass_sources}
  ${memoir_sources})

install(TARGETS ${PassName}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
