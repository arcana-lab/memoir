set(runtime_name "memoir.impl")
project(${runtime_name})

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

function(add_bitcode name)
  cmake_parse_arguments(
		SRC        # prefix of output variables
		""         # Boolean arguments
		""         # Single value arguments
		"SOURCES"  # multi value arguments
		${ARGN}    # arguments of the function to parse, here we take the all original ones
    )
  
  add_library(${runtime_name}_native STATIC ${SRC_SOURCES})
  target_include_directories(${name}_native PUBLIC include/)
  set_target_properties(${name}_native PROPERTIES
    LINKER_LANGUAGE CXX
    PUBLIC_HEADER "${HEADER_FILES}"
    PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

  install(
    TARGETS ${name}_native
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  llvmir_attach_bc_target(
    TARGET ${name}_bitcodes
    DEPENDS ${name}_native
    )

  llvmir_attach_link_target(
    TARGET ${name}
    DEPENDS ${name}_bitcodes
    OUTPUT_DIR ${CMAKE_INSTALL_LIBDIR}
    )

  set_property(
    TARGET ${name}
    PROPERTY
    EXCLUDE_FROM_ALL OFF
    )

endfunction()

add_bitcode(
  ${runtime_name}
  SOURCES
  ${SRC_FILES}
  )

add_subdirectory(backend)
add_subdirectory(api)
