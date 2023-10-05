set(BACKEND_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/backend)
set(BACKEND_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/backend)

add_subdirectory(vector)
add_subdirectory(hashtable)
add_subdirectory(stl_unordered_map)
add_subdirectory(stl_map)
