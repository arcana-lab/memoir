# Configure LLVM.
find_package(LLVM ${LLVM_VERSION} REQUIRED CONFIG)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "LLVM_DIR IS ${LLVM_CMAKE_DIR}.")

# Compiler options.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fPIC -g3 -Wno-unused-variable -Wno-reorder-ctor -fno-rtti ")

# Add dependencies.
include(ExternalProject)

# Configure NOELLE
ExternalProject_Add(
  noelle
  GIT_REPOSITORY  "https://github.com/arcana-lab/noelle.git"
  GIT_TAG         b6b3dda
  LOG_CONFIGURE ON
  LOG_INSTALL ON
  BUILD_COMMAND   ${CMAKE_COMMAND} --build . -j16
  INSTALL_COMMAND ${CMAKE_COMMAND} --install .
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DNOELLE_SVF=OFF
    -DNOELLE_SCAF=OFF
)

# Include dependencies
include_directories(
  ${LLVM_INCLUDE_DIRS} # LLVM
  ${CMAKE_CURRENT_SOURCE_DIR}/.. # MEMOIR
  ${CMAKE_INSTALL_INCLUDEDIR}
  ${CMAKE_INSTALL_INCLUDEDIR}/arcana # NOELLE
)

# Create the MEMOIR pass plugin.
function(create_llvm_pass_plugin)
  cmake_parse_arguments(
    SRC # prefix
    "" # booleans
    "TARGET" # single-value
    "DIRECTORIES" # multi-value
    ${ARGN} # arguments
  )

  # Add sources from each directory.
  set(SRC_FILES "")
  foreach(SRC_DIR ${SRC_DIRECTORIES})
    # Add the local source files to the target.
    file(
      GLOB_RECURSE LOCAL_FILES
      "${SRC_DIR}/*.c"
      "${SRC_DIR}/*.cpp"
      "${SRC_DIR}/*.cc"
    )
    list(APPEND SRC_FILES ${LOCAL_FILES})
  endforeach()

  # Create the target.
  add_llvm_pass_plugin(
    ${SRC_TARGET}
    MODULE
    ${SRC_FILES}
  )
endfunction()

# Compiler
create_llvm_pass_plugin(
  TARGET
  memoir

  DIRECTORIES
  llvm
  ir
  support
  utility
  analysis
  lower
  raise
  passes
  transform/utilities
  transform/normalization
  transform/statistics
  transform/profile_accesses
  transform/temp_arg_reification
  transform/sweep_selections
  transform/data_enumeration
)

# Add compile definitions to point to relevant directories.
target_compile_definitions(
  memoir
  PUBLIC "MEMOIR_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\""
  PUBLIC "LLVM_BIN_DIR=\"${LLVM_TOOLS_BINARY_DIR}\""
)
if(Boost_FOUND)
  target_compile_definitions(
    memoir
    PUBLIC "BOOST_INCLUDE_DIR=\"${Boost_INCLUDEDIR}\""
  )
endif()

# Add the noelle dependency.
add_dependencies(memoir noelle)

# Install the MEMOIR plugin.
install(TARGETS memoir LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Utility scripts
add_subdirectory(scripts)

# Runtime
add_subdirectory(op)
add_subdirectory(impl)
