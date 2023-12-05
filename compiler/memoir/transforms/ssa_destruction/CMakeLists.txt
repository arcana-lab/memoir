# Project
set(pass_name "memoir_ssa_destruction")

# Sources
file(GLOB pass_sources "src/*.cpp")

# Declare the LLVM pass to compile
add_llvm_library(
  ${pass_name}
  MODULE
  ${pass_sources}
  ${memoir_sources})

install(
  TARGETS
  ${PassName}
  LIBRARY
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR})
