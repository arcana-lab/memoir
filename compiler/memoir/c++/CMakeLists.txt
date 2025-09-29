# Get the module name.
get_filename_component(dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(header_files
  # Base objects
  object.hh
  element.hh
  collection.hh
  sequence.hh
  assoc.hh
  
  # Adaptors
  set.hh
  
  # Utilities
  counter.hh
  )

install(
  FILES ${header_files}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/memoir/${dir}
)

# Compatibility for old directory structure.
install(
  FILES ${header_files}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/memoir++
)
