add_llvm_library(
  memoir
  MODULE
  ${memoir_sources}
  )

function(add_memoir_sources)
  file(GLOB_RECURSE sources "*.cpp" "*.c" "*.cc")
  target_sources(
    memoir
    PUBLIC
    ${sources}
    )
endfunction()


# Install the include files.
set(dirs
  ir
  support
  utility
  analysis
  lowering
  )
foreach(dir ${dirs})
  # Fetch the header files.
  file(GLOB_RECURSE memoir_include ${dir} "*.hpp" "*.h" "*.def")

  # Install header files.
  install(
    FILES
    ${memoir_include}
    DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/memoir/${dir}
    )

  # Add the source files.
  add_subdirectory(${dir})
endforeach()

install(
  TARGETS memoir
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# set_property(GLOBAL PROPERTY MEMOIR_SOURCES "${memoir_sources}")

# Add all include directories
# get_property(memoir_sources GLOBAL PROPERTY MEMOIR_SOURCES)
add_subdirectory(transforms)
