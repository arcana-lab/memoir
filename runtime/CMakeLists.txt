project(ObjectIR)

set(CMAKE_C_FLAGS   "-O1 -Xclang -disable-llvm-passes -fdeclspec")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -g -march=native -mcmodel=large")

include_directories(include)

set(HEADER_FILES
  include/object_ir.h)

set(PRIVATE_HEADER_FILES
  include/objects.h
  include/types.h)

set(SRC_FILES
  src/accessor.cpp
  src/builder.cpp
  src/fields.cpp
  src/objects.cpp
  src/printer.cpp
  src/types.cpp)

add_library(object_ir_library STATIC ${SRC_FILES})
target_include_directories(object_ir_library PUBLIC include/)
set_target_properties(object_ir_library PROPERTIES
  LINKER_LANGUAGE CXX
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS object_ir_library
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET object_ir_bitcodes
  DEPENDS object_ir_library)

llvmir_attach_link_target(
  TARGET object_ir
  DEPENDS object_ir_bitcodes
  OUTPUT_DIR ${CMAKE_INSTALL_LIBDIR})

add_dependencies(object_ir object_ir_library)

set_property(TARGET object_ir_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET object_ir PROPERTY
  EXCLUDE_FROM_ALL OFF)
