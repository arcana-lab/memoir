# Pass
set(PassName "ObjectAbstraction")

# Sources
set(PassSrc
    Pass.cpp
    ObjectAbstraction.cpp
    TypeNode.cpp
    FieldNode.cpp
    ObjectNode.cpp
    Utils.cpp
)

# Compilation flags
set_source_files_properties(${PassSrc} PROPERTIES COMPILE_FLAGS " -std=c++17")

include_directories(${LLVM_INCLUDE_DIRS}
  ../include)

# Declare the LLVM pass to compile
add_llvm_library(${PassName} MODULE ${PassSrc})