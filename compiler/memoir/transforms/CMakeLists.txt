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
