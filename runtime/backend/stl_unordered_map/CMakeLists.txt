set(HEADER_FILES
  )

set(PRIVATE_HEADER_FILES
  )

set(SRC_FILES
  impl.cc
  )

add_library(backend_stl_unordered_map STATIC ${SRC_FILES})
target_include_directories(backend_stl_unordered_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(backend_stl_unordered_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set_target_properties(backend_stl_unordered_map PROPERTIES
  LINKER_LANGUAGE CXX
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS backend_stl_unordered_map
  ARCHIVE DESTINATION ${BACKEND_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${BACKEND_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET backend_stl_unordered_map_bitcodes
  DEPENDS backend_stl_unordered_map)

llvmir_attach_link_target(
  TARGET stl_unordered_map
  DEPENDS backend_stl_unordered_map_bitcodes
  OUTPUT_DIR ${BACKEND_INSTALL_LIBDIR})

add_dependencies(stl_unordered_map MemOIR)

set_property(TARGET backend_stl_unordered_map_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET stl_unordered_map PROPERTY
  EXCLUDE_FROM_ALL OFF)
