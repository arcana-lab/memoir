![memoir logo](memoir_logo.png)
A case for representing data collections and objects in an SSA form.

## Dependencies
The compiler depends on LLVM 18, it has been tested for LLVM 18.1.8
All other dependencies are installed automatically during the build process.

## Building
To build our noelle instance and the MEMOIR passes:
`make`

To build the unit tests:
`make test`

To clean the build:
`make clean`

To uninstall the existing build:
`make uninstall`

To clean the build and all dependencies:
`make fullclean`

To uninstall the existing build and all dependencies:
`make fulluninstall`

To build the Doxygen documentation:
`make documentation`
The results will be stored in `docs/build`.

## Contributing
For folks contributing, please ensure that you are using `clang-format` before pushing your changes.
There is a script in the top-level Makefile to setup a githook for this, but if you don't have clang-format installed, it won't work.

Also please familiarize yourself with the tools in `compiler/support/`.
It is expected that you use `MEMOIR_ASSERT` and its derivatives in place of a raw `assert`.
It is also expected that you use `memoir::println` instead of `llvm::errs()` or `std::cout`.
For more verbose outputs, use your judgement with either `memoir::infoln` or  `memoir::debugln`.

When formatting your git commit messages, please prefix with "[module1][module2]".
For example, if you make a change to `compiler/passes/memoir/ir/Instructions.hpp`, you should prepend "[compiler][ir]" to your commit message.

## Documentation
The [MEMOIR Developer Manual](http://mcmichen.cc/memoir-docs) provides a high-level view of MEMOIR, the compiler, and the supported language APIs.

The [MEMOIR Doxygen](http://mcmichen.cc/memoir-doxygen) provides source-level documentation of the MEMOIR compiler infrastructure.

Our [CGO'24 paper](http://mcmichen.cc/files/MEMOIR_CGO_2024.pdf) has additional information about MEMOIR.

If you use or build upon MEMOIR, we kindly ask that you cite us:
```
@inproceedings{MEMOIR:MCMICHEN:2024,
    title={Representing Data Collections in an SSA Form},
    author={McMichen, Tommy and Greiner, Nathan and Zhong, Peter and Sossai, Federico and Patel, Atmn and Campanoni, Simone},
    booktitle={CGO},
    year={2024},
    url={https://doi.org/10.1109/CGO57630.2024.10444817}
}
```
