# Configure LLVM 
find_package(LLVM ${LLVM_VERSION} REQUIRED CONFIG)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "LLVM_DIR IS ${LLVM_CMAKE_DIR}.")

# Compiler options.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fPIC -g3 -Wno-unused-variable -Wno-reorder-ctor -fno-rtti ")

include(ExternalProject)

# Configure clingo
ExternalProject_Add(
  clingo
  GIT_REPOSITORY "https://github.com/potassco/clingo"
  GIT_TAG v5.7.1
  LOG_CONFIGURE ON
  LOG_INSTALL ON
  BUILD_COMMAND ${CMAKE_COMMAND} --build . -j16
  INSTALL_COMMAND ${CMAKE_COMMAND} --install .
  CMAKE_ARGS
  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
  -DCMAKE_BUILD_TYPE=Release
  -DCLINGO_BUILD_STATIC=ON
  -DCLINGO_BUILD_EXAMPLES=ON
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
)

add_library(libclingo STATIC IMPORTED)
set_property(TARGET libclingo PROPERTY
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/clingo-prefix/src/clingo-build/lib/libclingo.a")

add_library(libclasp STATIC IMPORTED)
set_property(TARGET libclasp PROPERTY
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/clingo-prefix/src/clingo-build/lib/libclasp.a")

add_library(libgringo STATIC IMPORTED)
set_property(TARGET libgringo PROPERTY
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/clingo-prefix/src/clingo-build/lib/libgringo.a")

add_library(libpotassco STATIC IMPORTED)
set_property(TARGET libpotassco PROPERTY
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/clingo-prefix/src/clingo-build/lib/libpotassco.a")

add_library(libreify STATIC IMPORTED)
set_property(TARGET libreify PROPERTY
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/clingo-prefix/src/clingo-build/lib/libreify.a")


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

include_directories(
  # LLVM
  ${LLVM_INCLUDE_DIRS}
  # memoir dependencies
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_INSTALL_INCLUDEDIR}
  # External dependencies (NOELLE)
  ${CMAKE_INSTALL_INCLUDEDIR}/arcana
  # External dependencies (clingo)
  ${CMAKE_CURRENT_BINARY_DIR}/clingo-prefix/src/clingo/libclingo
  )

# Build the compiler.
add_subdirectory(memoir)
add_subdirectory(folio)
