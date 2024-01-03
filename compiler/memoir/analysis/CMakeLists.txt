# Get the module name.
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all header files.
# file(GLOB headers "*.hpp" "*.h" "*.hh" "*.def")

# Install header files.
install(
  FILES
  # ${headers}
  LivenessAnalysis.hpp
  TypeAnalysis.hpp
  
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/memoir/${name}
  )

# Collect all source files.
# file(GLOB sources "src/*.cpp" "src/*.c" "src/*.cc")

# Add the memoir sources to the core build.
add_memoir_sources(
  CORE
  FILES
  # ${sources}
  src/LivenessAnalysis.cpp
  src/TypeAnalysis.cpp
  )
