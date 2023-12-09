# Function to add a new memoir transform.
function(add_memoir_transform name)
  cmake_parse_arguments(
    SRC     # prefix
    ""  # booleans
    ""      # single-value
    "FILES" # multi-value
    ${ARGN} # arguments
    )

  add_memoir_sources(
    FILES
    ${SRC_FILES}
    )

  add_llvm_library(
    ${name}
    MODULE
    ${SRC_FILES}
    )

  install(
    TARGETS ${name}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endfunction()

# Core.
add_subdirectory(normalization)
add_subdirectory(mut_to_immut)
add_subdirectory(ssa_destruction)

# Utility passes.
add_subdirectory(statistics)
add_subdirectory(command_line)

# Transformation passes.
add_subdirectory(dead_collection_elimination)
add_subdirectory(dead_field_elimination)
add_subdirectory(field_elision)
add_subdirectory(key_folding)

# DEPRECATED passes.
# add_subdirectory(struct_of_arrays)
# add_subdirectory(slice_canonicalization)
# add_subdirectory(slice_propagation)
# add_subdirectory(sequence_provisioning)
# add_subdirectory(example)