#ifndef FOLIO_TRANSFORMS_ACCESSCOUNTER_H
#define FOLIO_TRANSFORMS_ACCESSCOUNTER_H

/**
 * This class will instrument an LLVM module to profile the types of accesses
 * that occur at run-time.
 */
struct AccessCounter {
public:
  AccessCounter(llvm::Module &M);
};

#endif // FOLIO_TRANSFORMS_ACCESSCOUNTER_H
