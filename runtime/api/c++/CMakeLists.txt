project(memoir-cpp C CXX ASM)

set(HEADER_FILES
  # Objects
  object.hh

  # Base collections
  collection.hh
  sequence.hh
  assoc.hh

  # Sequential collections
  
  # Assoc collections
  
  # Utilities
  counter.hh
  )

install(
  FILES ${HEADER_FILES}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/memoir++)
