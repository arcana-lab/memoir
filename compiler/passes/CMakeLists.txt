set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -g3 ")

# Configure subdirectories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(memoir)
#add_subdirectory(object_abstraction)
#add_subdirectory(object_inlining)
#add_subdirectory(object_lowering)
#add_subdirectory(struct_of_arrays)
#add_subdirectory(normalization)
add_subdirectory(example)
