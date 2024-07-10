add_llvm_library(
  memoir

  MODULE
  /dev/null

  PLUGIN_TOOL
  opt
)

function(add_memoir_sources)
  cmake_parse_arguments(
    SRC     # prefix
    ""  # booleans
    ""      # single-value
    "FILES" # multi-value
    ${ARGN} # arguments
    )
  
  if(NOT DEFINED SRC_FILES)
    file(GLOB_RECURSE SRC_FILES "*.cpp" "*.c" "*.cc")
  endif()

  target_sources(
    memoir
    PUBLIC
    ${SRC_FILES}
    )
endfunction()

add_subdirectory(ir)
add_subdirectory(support)
add_subdirectory(utility)
add_subdirectory(analysis)
add_subdirectory(lowering)
add_subdirectory(passes)
add_subdirectory(transforms)

install(
  TARGETS memoir
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
