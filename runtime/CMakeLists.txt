set(CMAKE_C_FLAGS   "-O1 -Xclang -disable-llvm-passes -c -emit-llvm")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -g -march=native -mcmodel=large")

include_directories(include)

add_library(ObjectIRLibrary STATIC
  src/accessor.cpp
  src/builder.cpp
  src/fields.cpp
  src/objects.cpp
  src/printer.cpp
  src/type_factory.cpp
  src/types.cpp)

set_target_properties(ObjectIRLibrary PROPERTIES LINKER_LANGUAGE CXX)

llvmir_attach_bc_target(
  TARGET ObjectIRBitcodes
  DEPENDS ObjectIRLibrary)

llvmir_attach_link_target(
  TARGET ObjectIR
  DEPENDS ObjectIRBitcodes
  OUTPUT_DIR ${CMAKE_INSTALL_LIBDIR})

set_property(TARGET ObjectIRBitcodes PROPERTY EXCLUDE_FROM_ALL OFF)
set_property(TARGET ObjectIR PROPERTY EXCLUDE_FROM_ALL OFF)

install(FILES
  include/object_ir.h
  include/objects.h
  include/types.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
