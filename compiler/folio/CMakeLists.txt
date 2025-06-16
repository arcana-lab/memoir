# Collect all header files.
file(GLOB headers "include/*.hpp" "include/*.h" "include/*.hh" "include/*.def")

# Install header files.
install(
  FILES
  ${headers}
  
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/folio/
)

# Collect all source files.
file(GLOB_RECURSE sources "src/*.cpp" "src/*.c" "src/*.cc")

# Add the sources to the build.
add_llvm_library(
  folio

  MODULE
  ${sources}

  PLUGIN_TOOL
  opt
)

include_directories(include)

add_dependencies(folio
  memoir
  noelle
)

add_subdirectory(scripts)

install(
  TARGETS folio
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
