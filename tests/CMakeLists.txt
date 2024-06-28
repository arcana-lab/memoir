file(GLOB_RECURSE headers "include/*.hpp" "include/*.h" "include/*.hh")

install(
  FILES
  ${headers}
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/cmemoir
  )

