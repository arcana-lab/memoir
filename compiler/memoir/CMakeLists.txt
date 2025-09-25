add_llvm_pass_plugin(memoir MODULE /dev/null)

add_dependencies(memoir noelle)

add_compile_definitions("MEMOIR_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
add_compile_definitions("LLVM_BIN_DIR=\"${LLVM_TOOLS_BINARY_DIR}\"")
if(Boost_FOUND)
  add_compile_definitions("BOOST_INCLUDE_DIR=\"${Boost_INCLUDEDIR}\"")
endif()

function(add_memoir_sources)
  cmake_parse_arguments(
    SRC # prefix
    "" # booleans
    "" # single-value
    "FILES" # multi-value
    ${ARGN} # arguments
  )

  if(NOT DEFINED SRC_FILES)
    file(GLOB_RECURSE SRC_FILES "*.cpp" "*.c" "*.cc")
  endif()

  target_sources(memoir PUBLIC ${SRC_FILES})
endfunction()

add_subdirectory(scripts)

add_subdirectory(llvm)
add_subdirectory(ir)
add_subdirectory(support)
add_subdirectory(utility)
add_subdirectory(analysis)
add_subdirectory(lowering)
add_subdirectory(raising)
add_subdirectory(passes)
add_subdirectory(transform)

install(TARGETS memoir LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
