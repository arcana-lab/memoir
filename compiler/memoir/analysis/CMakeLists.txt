# Get the module name.
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all header files.
file(GLOB_RECURSE headers "*.hpp" "*.h" "*.hh" "*.def")

# Install header files.
install(
  FILES
  ${headers}
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/memoir/${name}
  )

# Collect all source files.
file(GLOB_RECURSE sources "*.cpp" "*.c" "*.cc")

# Add the memoir sources to the core build.
add_memoir_sources(
  CORE
  FILES
  ${sources}
  )





#################################################################
# Temporarily add 'NoelleWrapper' pass here

set(PassName "NoelleWrapper")

add_llvm_library(
  ${PassName}
  MODULE
  src/NoelleWrapper.cpp
  )

install(TARGETS ${PassName}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#################################################################
