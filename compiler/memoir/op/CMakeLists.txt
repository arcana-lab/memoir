# Get the module name.
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(runtime_name "memoir-runtime")

set(CMAKE_C_FLAGS   "-O0 -Xclang -disable-llvm-passes -fdeclspec -fPIC -Wreturn-type")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++20 -g")

add_compile_definitions(_GLIBCXX_USE_CX11_ABI=0)

set(
  HEADER_FILES
  memoir.inc
  memoir.h
  utils.h
  types.def
)

install(
  FILES
  ${HEADER_FILES}
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/memoir/${name}
)

# Build the API runtime library.
add_library(
  ${runtime_name}-static
  STATIC
  memoir.c
)

set_target_properties(${runtime_name}-static PROPERTIES
  LINKER_LANGUAGE C
  PUBLIC_HEADER "${HEADER_FILES}"
)

install(
  TARGETS ${runtime_name}-static
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Build the API runtime bitcode.
llvmir_attach_bc_target(
  TARGET ${runtime_name}-bc
  DEPENDS ${runtime_name}-static
)

llvmir_attach_link_target(
  TARGET memoir.impl
  DEPENDS ${runtime_name}-bc
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

set_property(
  TARGET memoir.impl
  PROPERTY
  EXCLUDE_FROM_ALL OFF
)

# Extract the declarations.
set(IMPL_BC "${CMAKE_CURRENT_BINARY_DIR}/memoir.impl.bc")
set(DECL_BC "${CMAKE_CURRENT_BINARY_DIR}/memoir.decl.bc")
add_custom_command(
  OUTPUT ${DECL_BC}
  COMMAND llvm-extract -rglob "llvm.*.used" -o ${DECL_BC} ${IMPL_BC}
  COMMAND_EXPAND_LISTS VERBATIM
  DEPENDS memoir.impl
)

add_custom_target(
  ${runtime_name}-decl
  ALL
  DEPENDS
  ${DECL_BC}
)

install(
  FILES
  ${DECL_BC}
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}
)
