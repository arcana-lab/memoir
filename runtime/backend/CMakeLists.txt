set(BACKEND_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/backend)
set(BACKEND_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/backend)

set(CMAKE_C_FLAGS   "-O3 -g0 -fdeclspec -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17")

add_subdirectory(vector)
add_subdirectory(hashtable)
add_subdirectory(stl_unordered_map)
# add_subdirectory(stl_map)
add_subdirectory(deepsjeng_ttable)
add_subdirectory(stl_vector)

# Configure LLVM
find_package(LLVM 9 REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)
include_directories(${LLVM_INCLUDE_DIRS})
link_libraries(LLVMSupport)

set(LLVM_BACKEND_INSTALL_LIBDIR ${BACKEND_INSTALL_LIBDIR}/llvm)
set(LLVM_BACKEND_INSTALL_INCLUDEDIR ${BACKEND_INSTALL_INCLUDEDIR}/llvm)

add_subdirectory(llvm_smallvector)
add_subdirectory(llvm_densemap)
add_subdirectory(llvm_smallptrset)
