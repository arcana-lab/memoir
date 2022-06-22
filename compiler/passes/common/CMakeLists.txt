set(sources "")

set(CommonDirs
  support
  abstraction
  utility)

foreach (CommonDir ${CommonDirs})
  file(GLOB source "${CommonDir}/src/*.cpp")
  list(APPEND sources ${source}) 
endforeach ()

set(CommonSrc ${sources} PARENT_SCOPE)
