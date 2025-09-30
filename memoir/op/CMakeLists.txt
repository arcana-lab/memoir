# Get the module name.
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Configure compiler flags.
set(CMAKE_C_FLAGS   "-O0 -Xclang -disable-llvm-passes -fdeclspec -fPIC -Wreturn-type")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++20 -g")
add_compile_definitions(_GLIBCXX_USE_CX11_ABI=0)

# Build the API runtime library.
add_library(
  memoir-static
  STATIC
  memoir.c
)

set_target_properties(
  memoir-static
  PROPERTIES
  LINKER_LANGUAGE C
)

add_custom_target(
  memoir-defs
  DEPENDS
  memoir.def
)

add_dependencies(memoir-static memoir-defs)

# Build the API runtime bitcode.
llvmir_attach_bc_target(
  TARGET memoir-bitcodes
  DEPENDS memoir-static
)

llvmir_attach_link_target(
  TARGET memoir.impl
  DEPENDS memoir-bitcodes
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
  COMMAND llvm-extract -rglob "llvm.*used" -o ${DECL_BC} ${IMPL_BC}
  COMMAND_EXPAND_LISTS VERBATIM
  DEPENDS memoir.impl
)

add_custom_target(
  memoir.decl
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
