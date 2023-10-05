set(impl_name "stl_map")

set(HEADER_FILES
  )

set(PRIVATE_HEADER_FILES
  )

set(SRC_FILES
  impl.cc
  )
add_library(backend_${impl_name} STATIC ${SRC_FILES})
target_include_directories(backend_${impl_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(backend_${impl_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set_target_properties(backend_${impl_name} PROPERTIES
  LINKER_LANGUAGE CXX
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS backend_${impl_name}
  ARCHIVE DESTINATION ${BACKEND_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${BACKEND_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET backend_${impl_name}_bitcodes
  DEPENDS backend_${impl_name})

llvmir_attach_link_target(
  TARGET ${impl_name}
  DEPENDS backend_${impl_name}_bitcodes
  OUTPUT_DIR ${BACKEND_INSTALL_LIBDIR})

add_dependencies(${impl_name} MemOIR)

set_property(TARGET backend_${impl_name}_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET ${impl_name} PROPERTY
  EXCLUDE_FROM_ALL OFF)
