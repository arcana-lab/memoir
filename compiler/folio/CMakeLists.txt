add_llvm_library(
  folio

  MODULE
  /dev/null

  PLUGIN_TOOL
  opt
)

add_dependencies(folio
  memoir
  noelle
)

function(add_folio_sources)
  cmake_parse_arguments(
    SRC     # prefix
    ""      # booleans
    ""      # single-value
    "FILES" # multi-value
    ${ARGN} # arguments
    )
  
  if(NOT DEFINED SRC_FILES)
    file(GLOB_RECURSE SRC_FILES "*.cpp" "*.c" "*.cc")
  endif()

  target_sources(
    folio
    PUBLIC
    ${SRC_FILES}
    )
endfunction()

# add_subdirectory(scripts)

add_subdirectory(analysis)
add_subdirectory(transforms)
add_subdirectory(passes)

install(
  TARGETS folio
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
