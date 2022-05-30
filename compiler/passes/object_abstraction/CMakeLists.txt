# Project
set(ProjectName "ObjectAbstraction")
project(${ProjectName})

# Pass
add_subdirectory(src)

install(TARGETS ${ProjectName}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})