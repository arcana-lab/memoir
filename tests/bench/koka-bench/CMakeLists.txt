project(KokaBench)

option(BUILD_TESTING "" ON)
include(CTest)

enable_testing()

add_subdirectory(cpp)
#add_subdirectory(haskell)
#add_subdirectory(koka)
#add_subdirectory(ocaml)
#add_subdirectory(swift)
#add_subdirectory(java)
add_subdirectory(memoir)
