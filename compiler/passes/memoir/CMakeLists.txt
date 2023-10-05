set(dirs
  ir
  support
  utility
  analysis
  lowering
  )

set(memoir_include_dirs ${memoir_include_list})
foreach(dir ${dirs})
  # Install header files
  file(GLOB_RECURSE memoir_include ${CMAKE_CURRENT_SOURCE_DIR}/${dir} "*.[h|hpp|def]")
  install(FILES ${memoir_include}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/memoir/${dir})

  # Append this include dir for compilation
  list(APPEND memoir_include_dirs ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
endforeach()
set_property(GLOBAL PROPERTY MEMOIR_INCLUDE_DIRS "${memoir_include_dirs}")

# Add all include directories
get_property(memoir_include_dirs GLOBAL PROPERTY MEMOIR_INCLUDE_DIRS)
include_directories(${memoir_include_dirs})

# Add each Pass
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
