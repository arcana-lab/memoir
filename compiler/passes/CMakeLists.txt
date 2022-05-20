set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -g3 ")

# Configure subdirectories
include_directories(
  ../noelle/install/include
  ../noelle/install/include/svf
)

add_subdirectory(object_abstraction)
add_subdirectory(object_inlining)
add_subdirectory(object_lowering)
add_subdirectory(struct_of_arrays)
