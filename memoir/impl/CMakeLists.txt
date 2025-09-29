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

# Configure LLVM
# find_package(LLVM 9 REQUIRED CONFIG)
# list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
# include(HandleLLVMOptions)
# include(AddLLVM)
# include_directories(${LLVM_INCLUDE_DIRS})
# link_libraries(LLVMSupport)
