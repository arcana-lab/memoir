# Collect all source files.
add_memoir_sources()

# Temporarily add 'NoelleWrapper' pass here

set(PassName "NoelleWrapper")

add_llvm_library(
  ${PassName}
  MODULE
  src/NoelleWrapper.cpp
  )

install(TARGETS ${PassName}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
