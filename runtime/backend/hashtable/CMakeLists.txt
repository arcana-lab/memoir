set(HEADER_FILES
  )

set(PRIVATE_HEADER_FILES
  )

set(SRC_FILES
  impl.c
  )

add_library(backend_hashtable STATIC ${SRC_FILES})
target_include_directories(backend_hashtable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(backend_hashtable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set_target_properties(backend_hashtable PROPERTIES
  LINKER_LANGUAGE C
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS backend_hashtable
  ARCHIVE DESTINATION ${BACKEND_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${BACKEND_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET backend_hashtable_bitcodes
  DEPENDS backend_hashtable)

llvmir_attach_link_target(
  TARGET hashtable
  DEPENDS backend_hashtable_bitcodes
  OUTPUT_DIR ${BACKEND_INSTALL_LIBDIR})

add_dependencies(hashtable MemOIR)

set_property(TARGET backend_hashtable_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET hashtable PROPERTY
  EXCLUDE_FROM_ALL OFF)
