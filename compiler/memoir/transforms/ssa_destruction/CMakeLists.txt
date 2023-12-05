# Project
set(pass_name "memoir_ssa_destruction")

# Sources
file(GLOB pass_sources "src/*.cpp")

# Declare the LLVM pass to compile
add_memoir_transform(
  ${pass_name}
  FILES
  ${pass_sources}
  )
