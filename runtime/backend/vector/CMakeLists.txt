set(impl "vector")

set(HEADER_FILES
  ${impl}.h
  )

set(PRIVATE_HEADER_FILES
  )

set(SRC_FILES
  impl.c
  )

add_library(backend_vector STATIC ${SRC_FILES})
target_include_directories(backend_vector PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(backend_vector PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set_target_properties(backend_vector PROPERTIES
  LINKER_LANGUAGE C
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS backend_vector
  ARCHIVE DESTINATION ${BACKEND_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${BACKEND_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET backend_vector_bitcodes
  DEPENDS backend_vector)

llvmir_attach_link_target(
  TARGET vector
  DEPENDS backend_vector_bitcodes
  OUTPUT_DIR ${BACKEND_INSTALL_LIBDIR})

add_dependencies(vector ${runtime_name})

set_property(TARGET backend_vector_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET vector PROPERTY
  EXCLUDE_FROM_ALL OFF)
