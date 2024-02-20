add_llvm_library(memoir MODULE /dev/null)

add_llvm_library(memoir_core MODULE /dev/null)

function(add_memoir_sources)
  cmake_parse_arguments(
    SRC     # prefix
    "CORE"  # booleans
    ""      # single-value
    "FILES" # multi-value
    ${ARGN} # arguments
    )
  
  if(NOT DEFINED SRC_FILES)
    file(GLOB_RECURSE SRC_FILES "*.cpp" "*.c" "*.cc")
  endif()

  if(${SRC_CORE})
    target_sources(
      memoir_core
      PUBLIC
      ${SRC_FILES}
      )
  endif()
  
  target_sources(
    memoir
    PUBLIC
    ${SRC_FILES}
    )
endfunction()


# Install the include files.
add_subdirectory(ir)
add_subdirectory(support)
add_subdirectory(utility)
add_subdirectory(analysis)
add_subdirectory(lowering)

install(
  TARGETS memoir_core
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

# set_property(GLOBAL PROPERTY MEMOIR_SOURCES "${memoir_sources}")

# Add all include directories
# get_property(memoir_sources GLOBAL PROPERTY MEMOIR_SOURCES)
add_subdirectory(transforms)

install(
  TARGETS memoir
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
