# Install scripts.
add_subdirectory(scripts)

# Configure LLVM 
find_package(LLVM 18.1.6 REQUIRED CONFIG)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "LLVM_DIR IS ${LLVM_CMAKE_DIR}.")

# Configure NOELLE
include(ExternalProject)
ExternalProject_Add(
    noelle
    GIT_REPOSITORY  "https://github.com/arcana-lab/noelle.git"
    GIT_TAG         v18
    LOG_CONFIGURE ON
    LOG_INSTALL ON
    LOG_BUILD ON
    LOG_OUTPUT_ON_FAILURE ON
    BUILD_COMMAND   ${CMAKE_COMMAND} --build . -j16
    INSTALL_COMMAND ${CMAKE_COMMAND} --install .
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DNOELLE_SVF=OFF
        -DNOELLE_SCAF=OFF
)

# Compiler options.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -g3 -Wno-unused-variable -Wno-reorder-ctor ")

include_directories(
  # LLVM
  ${LLVM_INCLUDE_DIRS}
  # memoir dependencies
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_INSTALL_INCLUDEDIR}
  # External dependencies (NOELLE)
  ${CMAKE_INSTALL_INCLUDEDIR}/arcana
  )

link_libraries(${Z3_LIBRARIES})

# Build the compiler.
add_subdirectory(memoir)

