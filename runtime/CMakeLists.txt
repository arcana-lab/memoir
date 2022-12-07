project(MemOIR)

set(CMAKE_C_FLAGS   "-O1 -Xclang -disable-llvm-passes -fdeclspec")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -g -march=native -mcmodel=large")

include_directories(include)

set(HEADER_FILES
  include/memoir.h)

set(PRIVATE_HEADER_FILES
  include/objects.h
  include/types.h
  include/utils.h)

set(SRC_FILES
  src/accessor.cpp
  src/builder.cpp
  src/elements.cpp
  src/objects.cpp
  src/printer.cpp
  src/types.cpp)

add_library(memoir_library STATIC ${SRC_FILES})
target_include_directories(memoir_library PUBLIC include/)
set_target_properties(memoir_library PROPERTIES
  LINKER_LANGUAGE CXX
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS memoir_library
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET memoir_bitcodes
  DEPENDS memoir_library)

llvmir_attach_link_target(
  TARGET memoir
  DEPENDS memoir_bitcodes
  OUTPUT_DIR ${CMAKE_INSTALL_LIBDIR})

add_dependencies(memoir memoir_library)

set_property(TARGET memoir_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET memoir PROPERTY
  EXCLUDE_FROM_ALL OFF)
