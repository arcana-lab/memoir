# Configure z3
find_package(Z3 CONFIG REQUIRED HINTS /project/tommy-software/z3/z3-4.12.2/install/lib64)

# Configure LLVM 
find_package(LLVM 9 REQUIRED CONFIG)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR}/)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "LLVM_DIR IS ${LLVM_CMAKE_DIR}.")

message(STATUS "Z3_LIBRARIES=${Z3_LIBRARIES}")

include_directories(
  ${Z3_INCLUDE_DIR}
  ${LLVM_INCLUDE_DIRS}
  noelle/install/include
  noelle/install/include/svf)

link_libraries(${Z3_LIBRARIES})

add_subdirectory(passes)
