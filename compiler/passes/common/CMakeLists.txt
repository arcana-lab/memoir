set(CommonSrc "")

set(CommonDirs
  support
  abstraction
  utility)

foreach (CommonDir ${CommonDirs})
  list(APPEND CommonSrc "${CommonDir}/src/*.cpp") 
endforeach ()
