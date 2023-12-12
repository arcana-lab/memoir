# Sources
file(GLOB sources "src/*.cpp")

# Add command line utility to memoir core.
add_memoir_sources(
  CORE
  FILES
  ${sources} 
)
