# Project
set(PassName "ObjectAbstraction")
project(${PassName})

# Sources
file(GLOB PassSrc "src/*.cpp")

# Compilation flags
set_source_files_properties(${PassSrc} PROPERTIES COMPILE_FLAGS " -std=c++17")

# Declare the LLVM pass to compile
add_llvm_library(
  ${PassName}
  MODULE
  ${PassSrc}
  ${CommonSrc}
)

install(TARGETS ${PassName}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

