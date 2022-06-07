set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(sources
  rbtree/rbtree.cpp
  # rbtree-ck/rbtree-ck.cpp
  nqueens/nqueens.cpp
  deriv/deriv.cpp
  cfold/cfold.cpp
  # binarytrees/binarytrees.cpp
  )

foreach (source ${sources})
  get_filename_component(name "${source}" NAME_WE)
  set(name "cpp-${name}")

  add_executable(${name} ${source})
  set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
  
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES LABELS cpp)
endforeach ()

# target_compile_options(cpp-rbtreec PUBLIC -fpermissive)
