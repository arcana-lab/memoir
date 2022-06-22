# Common Utilities, Abstractions and Tools for MemOIR passes
## Using a tool?
Make sure that your pass's CMakeList includes `${CommonSrc}` in its required source files.
When including the tool in your pass, add `#include "common/support/MyTool.hpp"`, you must include the full path to the common header you want.

## Adding a new tool?
If adding it to an existsing subdirectory, add the top-level header file to the directory it belongs and place the source files in the respective `src/` dir.
Follow the file structure described below:

## File Structure
The build system assumes this file structure.
```
common
|-- utility
|   |-- MyTool.hpp
|   |-- src
|       |-- MyTool.cpp
|       |-- MyToolHelper.cpp
|-- support
|   |-- ThisSupport.hpp
|   |-- OtherSupport.hpp
|   |-- src
|       |-- ThisSupport.cpp
|       |-- OtherSupport.cpp
|-- abstraction
    |-- ...
```

