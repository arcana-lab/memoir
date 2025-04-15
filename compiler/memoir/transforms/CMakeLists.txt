# Function to add a new memoir transform.
function(add_memoir_transform name)
  cmake_parse_arguments(
    SRC     # prefix
    ""      # booleans
    ""      # single-value
    "FILES" # multi-value
    ${ARGN} # arguments
  )

  add_memoir_sources(
    FILES
    ${SRC_FILES}
  )
endfunction()

# Core.
add_subdirectory(utilities)
add_subdirectory(normalization)
add_subdirectory(ssa_construction)
add_subdirectory(ssa_destruction)
add_subdirectory(live_out_insertion)
add_subdirectory(type_inference)

# Utility passes.
add_subdirectory(statistics)
add_subdirectory(profile_accesses)

# Transformation passes.
# add_subdirectory(dead_collection_elimination)
# add_subdirectory(dead_field_elimination)
# add_subdirectory(field_elision)
# add_subdirectory(key_folding)
# add_subdirectory(dead_element_elimination)

# Lowering passes.
add_subdirectory(impl_linker)
add_subdirectory(temp_arg_reification)
