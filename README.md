![memoir logo](memoir_logo.png)
A case for memory object representation in the LLVM IR

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


## TODOs
- [ ] Support for static length tensors (on `static-tensor` branch)
  - [x] Add to runtime
  - [x] Add to common analyses
  - [ ] Add to object lowering
- [ ] Support for copy/clone (on `copy-and-clone` branch)
  - [ ] Ensure that this is needed by benchmarks
