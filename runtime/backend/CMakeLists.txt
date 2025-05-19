set(BACKEND_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/backend)
set(BACKEND_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/backend)

set(CMAKE_C_FLAGS   "-O3 -gdwarf-4 -fdeclspec -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17")

# Find or install Abseil.
find_package(absl)
if (NOT absl_FOUND)
  message(WARNING "Failed to find Abseil, installing.")

  include(ExternalProject)
  ExternalProject_Add(
    absl
    GIT_REPOSITORY "git@github.com:abseil/abseil-cpp.git"
    GIT_TAG lts_2025_01_27
    LOG_CONFIGURE ON
    LOG_INSTALL ON
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  )
endif()


# Install Roaring.
find_package(roaring)
if (NOT roaring_FOUND)
  message(WARNING "Failed to find Roaring, installing.")

  include(ExternalProject)
  ExternalProject_Add(
    roaring
    GIT_REPOSITORY "git@github.com:RoaringBitmap/CRoaring.git"
    GIT_TAG v4.3.1
    LOG_CONFIGURE ON
    LOG_INSTALL ON
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  )
endif()

# Install the header files.
install(
  FILES utilities.h
  DESTINATION ${BACKEND_INSTALL_INCLUDEDIR}
)

function(install_impl DIR)
  install(
    FILES
    ${DIR}/definition.hpp
    ${DIR}/declaration.h
    ${DIR}/instantiation.h
    DESTINATION
    ${BACKEND_INSTALL_INCLUDEDIR}/${DIR}
  )
endfunction()

# C++ standard template library.
install_impl(stl_unordered_map)
# install_impl(stl_map)
install_impl(stl_multimap)
install_impl(stl_vector)
install_impl(stl_unordered_set)

# Boost library.
install_impl(boost_flat_multimap)
install_impl(boost_flat_set)
install_impl(boost_flat_map)

# Custom bit{set,map} implementations.
install_impl(bitset)
install_impl(bitmap)
install_impl(sparse_bitset)
install_impl(sparse_bitmap)
install_impl(twined_bitmap)

# Abseil containers
install_impl(abseil_flat_hash_set)
install_impl(abseil_flat_hash_map)

# Configure LLVM
# find_package(LLVM 9 REQUIRED CONFIG)
# list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
# include(HandleLLVMOptions)
# include(AddLLVM)
# include_directories(${LLVM_INCLUDE_DIRS})
# link_libraries(LLVMSupport)

# set(LLVM_BACKEND_INSTALL_LIBDIR ${BACKEND_INSTALL_LIBDIR}/llvm)
# set(LLVM_BACKEND_INSTALL_INCLUDEDIR ${BACKEND_INSTALL_INCLUDEDIR}/llvm)

# install_impl(llvm_smallvector)
# install_impl(llvm_densemap)
# install_impl(llvm_smallptrset)
