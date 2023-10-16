project(MemOIR-cpp)

set(HEADER_FILES
  # Objects
  object.hh

  # Base collections
  collection.hh
  sequence.hh
  assoc.hh

  # Sequential collections
  array.hh
  vector.hh
  list.hh
  forward_list.hh
  
  # Assoc collections
  
  # Utilities
  counter.hh
  )

install(
  FILES ${HEADER_FILES}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/memoir++)
