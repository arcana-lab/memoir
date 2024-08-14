# Get the module name.
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all header files.
file(GLOB headers "*.hpp" "*.h" "*.hh" "*.def")

# Install header files.
install(
  FILES
  ${headers}
  
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/folio/${name}
)

# Collect all source files.
file(GLOB_RECURSE sources "src/*.cpp" "src/*.c" "src/*.cc")

# Add the memoir sources to the core build.
add_folio_sources(
  FILES
  ${sources}
)
