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

# Utility passes.
add_subdirectory(statistics)

# Transformation passes.
add_subdirectory(dead_collection_elimination)
add_subdirectory(dead_field_elimination)
add_subdirectory(field_elision)
add_subdirectory(key_folding)
add_subdirectory(type_inference)
add_subdirectory(dead_element_elimination)

# Lowering passes.
add_subdirectory(impl_linker)
add_subdirectory(lower_fold)

# Analysis passes.
add_subdirectory(example)
