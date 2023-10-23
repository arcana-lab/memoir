project(MemOIR)

set(CMAKE_C_FLAGS   "-O1 -Xclang -disable-llvm-passes -fdeclspec -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -g")

include_directories(include)

set(HEADER_FILES
  include/memoir.h
  )

set(PRIVATE_HEADER_FILES
  include/objects.h
  include/types.h
  include/utils.h
  include/types.def)

set(SRC_FILES
  # General-purpose
  src/allocation.cpp
  src/deletion.cpp
  src/type.cpp
  src/type_checking.cpp
  
  # SSA
  src/ssa_access.cpp
  src/ssa_operations.cpp
  
  # Mut
  src/mut_access.cpp
  src/mut_operations.cpp
  
  # Internal
  src/detail/elements.cpp
  src/detail/objects.cpp
  src/detail/collections.cpp
  src/detail/printer.cpp
  src/detail/types.cpp
  )

add_library(MemOIR STATIC ${SRC_FILES})
target_include_directories(MemOIR PUBLIC include/)
set_target_properties(MemOIR PROPERTIES
  LINKER_LANGUAGE CXX
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS MemOIR
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

llvmir_attach_bc_target(
  TARGET memoir_bitcodes
  DEPENDS MemOIR)

llvmir_attach_link_target(
  TARGET memoir
  DEPENDS memoir_bitcodes
  OUTPUT_DIR ${CMAKE_INSTALL_LIBDIR})

add_dependencies(memoir MemOIR)

set_property(TARGET memoir_bitcodes PROPERTY
  EXCLUDE_FROM_ALL OFF)
set_property(TARGET memoir PROPERTY
  EXCLUDE_FROM_ALL OFF)

add_subdirectory(backend)
add_subdirectory(api)
