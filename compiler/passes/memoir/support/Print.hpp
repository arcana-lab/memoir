#ifndef MEMOIR_PRINT_H
#define MEMOIR_PRINT_H
#pragma once

#include "llvm/Support/raw_ostream.h"

/*
 * This file provides utility functions for printing.
 *
 * Author(s): Tommy McMichen
 * Created: December 19, 2022
 */
namespace llvm::memoir {

// Printing.
inline void fprint(llvm::raw_ostream &out) {}
template <class T, class... Ts>
inline void fprint(llvm::raw_ostream &out, T const &first, Ts const &... rest) {
  out << first;
  fprint(out, rest...);
}

template <class... Ts>
inline void fprintln(llvm::raw_ostream &out, Ts const &... args) {
  fprint(out, args..., '\n');
}

template <class... Ts>
inline void println(Ts const &... args) {
  fprintln(llvm::errs(), args...);
}

template <class... Ts>
inline void print(Ts const &... args) {
  fprint(llvm::errs(), args...);
}

// Warnings.
template <class... Ts>
inline void fwarn(llvm::raw_ostream &out, Ts const &... args) {
  if (out.has_colors()) {
    out.changeColor(llvm::raw_ostream::Colors::YELLOW, /*bold=*/true);
  }

  fprint(out, "WARNING: ");

  if (out.has_colors()) {
    out.resetColor();
  }

  fprint(out, args...);
}

template <class... Ts>
inline void fwarnln(llvm::raw_ostream &out, Ts const &... args) {
  fwarn(out, args..., '\n');
}

template <class... Ts>
inline void warnln(Ts const &... args) {
  fwarnln(llvm::errs(), args...);
}

template <class... Ts>
inline void warn(Ts const &... args) {
  fwarn(llvm::errs(), args...);
}

} // namespace llvm::memoir
#endif
