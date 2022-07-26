set(CommonDirs
  support
  abstraction
  utility)

file(GLOB_RECURSE sources "*.cpp")

set(CommonSrc ${sources} PARENT_SCOPE)
