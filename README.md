![memoir logo](memoir_logo.png)
A case for memory object representation in the LLVM IR

## Dependencies
The compiler depends on z3.

## Building
To build our noelle instance and the MemOIR passes:
`make`

To build the unit tests:
`cd tests/unit ; make`

To clean the build:
`make clean`

To uninstall the existing build:
`make uninstall`

## Contributing
For folks contributing, please ensure that you are using `clang-format` before pushing your changes.
There is a script in the top-level Makefile to setup a githook for this, but if you don't have clang-format installed, it won't work.

Also please familiarize yourself with the tools in `compiler/support/`.
It is expected that you use `MEMOIR_ASSERT` and its derivatives in place of a raw `assert`.
It is also expected that you use `memoir::println` instead of `llvm::errs()` or `std::cout`.
For more verbose outputs, use your judgement with either `memoir::infoln` or  `memoir::debugln`.

When formatting your git commit messages, please prefix with "[module1][module2]".
For example, if you make a change to `compiler/passes/memoir/ir/Instructions.hpp`, you should prepend "[compiler][ir]" to your commit message.

## Additional Information
Our CGO'24 paper has additional information about MemOIR, you can find it [here](http://mcmichen.cc/files/MEMOIR_CGO_2024.pdf).

If you use or build upon MemOIR, we kindly ask that you cite us:
```
@inproceedings(MCMICHEN:2024:MEMOIR,
    title={Representing Data Collections in an SSA Form},
    author={McMichen, Tommy and Greiner, Nathan and Zhong, Peter and Sossai, Federico and Patel, Atmn and Campanoni, Simone},
    booktitle={International Symposium on Code Generation and Optimization, 2024. CGO 2024.},
    year={2024},
}
```
