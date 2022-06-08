set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(C_FLAGS   -O1 -Xclang -disable-llvm-passes -fdeclspec)
set(CXX_FLAGS ${C_FLAGS} -std=c++17 -g -march=native -mcmodel=large)

set(sources
  rbtree/rbtree.cpp
  # rbtree-ck/rbtree-ck.cpp
  nqueens/nqueens.cpp
  # deriv/deriv.cpp # NOT IMPLEMENTED YET
  cfold/cfold.cpp
  # binarytrees/binarytrees.cpp
  )

add_custom_target(bitcodes)

foreach (source ${sources})
  get_filename_component(dir_name "${source}" NAME_WE)
  set(name "objectir-${dir_name}")

  add_executable(${name} ${source})
  add_dependencies(${name} object_ir_library)
  
  set_target_properties(${name} PROPERTIES
    LINKER_LANGUAGE CXX)
  
  target_compile_options(${name} PUBLIC ${CXX_FLAGS})
  target_include_directories(${name} PUBLIC ${CMAKE_INSTALL_INCLUDEDIR})
  target_link_libraries(${name} PUBLIC object_ir_library)
  
  set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/build)
  
  set(bitcodes "${dir_name}_bitcodes")
  set(bitcodes_link "${dir_name}_in_one")
  llvmir_attach_bc_target(
    TARGET ${bitcodes}
    DEPENDS ${name})

  set_property(TARGET ${bitcodes} PROPERTY EXCLUDE_FROM_ALL true)
  
  llvmir_attach_link_target(
    TARGET ${bitcodes_link}
    DEPENDS ${bitcodes}
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/build)

  set_property(TARGET ${bitcodes_link} PROPERTY EXCLUDE_FROM_ALL true)

  add_dependencies(bitcodes ${bitcodes_link})

  add_test(NAME ${name}
    COMMAND ${name})
  set_tests_properties(${name} PROPERTIES
    LABELS cpp)

endforeach ()

# target_compile_options(cpp-rbtreec PUBLIC -fpermissive)
