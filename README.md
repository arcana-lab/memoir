# MemOIR
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

## TODOs
- [ ] Support for static length tensors (on `static-tensor` branch)
  - [ ] Add to runtime
  - [ ] Add to common analyses
  - [ ] Add to object lowering
- [ ] Support for copy/clone (on `copy-and-clone` branch)
  - [ ] Ensure that this is needed by benchmarks
