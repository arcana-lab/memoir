set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(C_FLAGS   -O1 -Xclang -disable-llvm-passes -fdeclspec)
set(CXX_FLAGS ${C_FLAGS} -std=c++17 -g -march=native -mcmodel=large)

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

set(sources */*.cpp)

SUBDIRLIST(subdirs ${CMAKE_CURRENT_SOURCE_DIR})
foreach (subdir ${subdirs})
  set(name "unit_${subdir}")
  file(GLOB sources ${subdir}/*.cpp)
  
  add_executable(${name} ${sources})
  add_dependencies(${name} object_ir_library)
  
  set_target_properties(${name} PROPERTIES
    LINKER_LANGUAGE CXX)
  
  target_compile_options(${name} PUBLIC ${CXX_FLAGS})
  target_include_directories(${name} PUBLIC ${CMAKE_INSTALL_INCLUDEDIR})
  target_link_libraries(${name} PUBLIC object_ir_library)
  
  set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/build)
  
  add_test(NAME ${name}
    COMMAND ${name})
  set_tests_properties(${name} PROPERTIES
    LABELS cpp)

endforeach ()

# target_compile_options(cpp-rbtreec PUBLIC -fpermissive)
