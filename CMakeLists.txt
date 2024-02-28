cmake_minimum_required(VERSION 3.13)

project(MEMOIR C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)

set(LLVM_VERSION 9.0.0)
set(CLANG_ID "Clang")

message("Compiling MEMOIR for LLVM version ${LLVM_VERSION}")

# Check that we are compiling with clang.
if (NOT CMAKE_C_COMPILER_ID STREQUAL CLANG_ID)
  message(FATAL_ERROR
    "C compiler ID is not clang!\n"
    "    Found compiler id ${CMAKE_CXX_COMPILER_ID}."
    " Ensure that your PATH is set correctly."
    " If it is not installed, do so at: https://releases.llvm.org/${LLVM_VERSION}/")
endif()

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL CLANG_ID)
  message(FATAL_ERROR
    "C++ compiler ID is not clang!"
    "    Found compiler id ${CMAKE_CXX_COMPILER_ID}."
    " Ensure that your PATH is set correctly."
    " If it is not installed, do so at: https://releases.llvm.org/${LLVM_VERSION}/")
endif()


# Check that clang/clang++ we are using is the correct version.
if (NOT CMAKE_C_COMPILER_VERSION VERSION_EQUAL LLVM_VERSION)
  message(FATAL_ERROR
    "Clang version is not ${LLVM_VERSION}!"
    "    Found version ${CMAKE_C_COMPILER_VERSION}."
    " Ensure that your PATH is set correctly."
    " If it is not installed, do so at: https://releases.llvm.org/${LLVM_VERSION}/")
endif()

if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL LLVM_VERSION)
  message(FATAL_ERROR
    "Clang++ version is not ${LLVM_VERSION}!"
    "    Found version ${CMAKE_CXX_COMPILER_VERSION}"
    " Ensure that your PATH is set correctly."
    " If it is not installed, do so at: https://releases.llvm.org/${LLVM_VERSION}/")
endif()

# Set up the build and install paths.
set(CMAKE_STAGING_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)

# Include the LLVMIR CMake utilities.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.cmake")
include(LLVMIRUtil)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#find_program(CCACHE_PROGRAM ccache)
#if(CCACHE_PROGRAM)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
#endif()

add_subdirectory(runtime)
add_subdirectory(compiler)

# Enable file.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/.enable.in ${CMAKE_CURRENT_SOURCE_DIR}/enable @ONLY)

# Documentation
option(BUILD_DOC "Build documentation" ON)
add_subdirectory(docs)
