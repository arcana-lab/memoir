set(CMAKE_C_FLAGS   "-O1 -Xclang -disable-llvm-passes -fdeclspec -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -g")

set(HEADER_FILES
  cmemoir.h
  mstring.h
  )

set(PRIVATE_HEADER_FILES
  )

set(SRC_FILES
  src/mstring.cpp
  )

add_library(cmemoir STATIC ${SRC_FILES})
target_include_directories(cmemoir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(cmemoir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set_target_properties(cmemoir PROPERTIES
  LINKER_LANGUAGE CXX
  PUBLIC_HEADER "${HEADER_FILES}"
  PRIVATE_HEADER "${PRIVATE_HEADER_FILES}")

install(
  TARGETS cmemoir
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmemoir
  PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmemoir)
