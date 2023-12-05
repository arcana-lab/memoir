set(dirs
  ir
  support
  utility
  analysis
  lowering
  )

# Install the include files.
set(memoir_include_dirs ${memoir_include_list})
foreach(dir ${dirs})
  # Install header files
  file(GLOB_RECURSE memoir_include ${dir} "*.hpp" "*.h" "*.def")
  install(
    FILES
    ${memoir_include}
    DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/memoir/${dir}
    )
endforeach()

# Collect the memoir source files.
set(memoir_sources ${memoir_source_list})
foreach(dir ${dirs})
  # Add source files
  file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/${dir} "*.cpp")

  # Append the sources to the list for compilation
  list(APPEND memoir_sources ${sources})
endforeach()

set_property(GLOBAL PROPERTY MEMOIR_SOURCES "${memoir_sources}")

# Add all include directories
get_property(memoir_sources GLOBAL PROPERTY MEMOIR_SOURCES)
