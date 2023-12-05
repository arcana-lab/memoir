# Pass
set(pass_name "memoir_command_line")

# Sources
file(GLOB pass_sources "src/*.cpp")

# Declare the LLVM pass to compile
add_memoir_transform(
  ${pass_name}
  CORE
  FILES
  ${pass_sources} 
)
