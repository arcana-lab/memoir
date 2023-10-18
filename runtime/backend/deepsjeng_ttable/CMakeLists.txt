set(impl "deepsjeng_ttable")

set(HEADER_FILES
  ${impl}.h
  )

set(PRIVATE_HEADER_FILES
  )

set(SRC_FILES
  impl.cc
  )

add_library(backend_${impl} STATIC ${SRC_FILES})
target_include_directories(backend_${impl} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(backend_${impl} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set_target_properties(backend_${impl} PROPERTIES
  LINKER_LANGUAGE CXX
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS backend_${impl}
  ARCHIVE DESTINATION ${BACKEND_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${BACKEND_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${BACKEND_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET backend_${impl}_bitcodes
  DEPENDS backend_${impl})

llvmir_attach_link_target(
  TARGET ${impl}
  DEPENDS backend_${impl}_bitcodes
  OUTPUT_DIR ${BACKEND_INSTALL_LIBDIR})

add_dependencies(${impl} MemOIR)

set_property(TARGET backend_${impl}_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET ${impl} PROPERTY
  EXCLUDE_FROM_ALL OFF)
