
# Install scripts.
add_subdirectory(scripts)

# Configure LLVM 
find_package(LLVM 9 REQUIRED CONFIG)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "LLVM_DIR IS ${LLVM_CMAKE_DIR}.")

# Compiler options.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -g3 ")

include_directories(
  # LLVM
  ${LLVM_INCLUDE_DIRS}
  # memoir dependencies
  ${CMAKE_CURRENT_SOURCE_DIR}
  # External dependencies (NOELLE)
  ${CMAKE_INSTALL_INCLUDEDIR}
  )

link_libraries(${Z3_LIBRARIES})

# Build the compiler.
add_subdirectory(memoir)

