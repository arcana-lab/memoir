# Pass
set(pass_name "memoir_slice_propagation")

# Sources
file(GLOB pass_sources "src/*.cpp")

get_property(memoir_sources GLOBAL PROPERTY MEMOIR_SOURCES)

# Declare the LLVM pass to compile
add_llvm_library(
  ${pass_name}
  MODULE
  ${pass_sources}
  ${memoir_sources}
  )

install(
  TARGETS ${PassName}
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
