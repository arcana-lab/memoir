# Project
cmake_minimum_required(VERSION 3.13)
project(CAT)

# Dependences
include(${CMAKE_CURRENT_SOURCE_DIR}/../../noelle/src/scripts/DependencesCMake.txt)
include_directories(include)

# Pass
add_subdirectory(src)
