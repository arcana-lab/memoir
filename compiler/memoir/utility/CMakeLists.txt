# Collect all source files.
file(GLOB_RECURSE sources "*.cpp" "*.c" "*.cc")

# Add the memoir sources to the core build.
add_memoir_sources(
  CORE
  FILES
  ${sources}
  )
