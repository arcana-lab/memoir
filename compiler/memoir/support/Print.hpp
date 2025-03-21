#ifndef MEMOIR_PRINT_H
#define MEMOIR_PRINT_H

#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

#include "llvm/Support/raw_ostream.h"

/*
 * This file provides utility functions for printing.
 *
 * Author(s): Tommy McMichen
 * Created: December 19, 2022
 */
namespace llvm::memoir {

enum Verbosity { noverbosity, quick, detailed };
extern Verbosity VerboseLevel;

// Printing.
inline void fprint(llvm::raw_ostream &out) {}
template <class T, class... Ts>
inline void fprint(llvm::raw_ostream &out, T const &first, Ts const &...rest) {
  out << first;
  fprint(out, rest...);
}

template <class... Ts>
inline void fprintln(llvm::raw_ostream &out, Ts const &...args) {
  fprint(out, args..., '\n');
}

template <class... Ts>
inline void println(Ts const &...args) {
  fprintln(llvm::errs(), args...);
}

template <class... Ts>
inline void print(Ts const &...args) {
  fprint(llvm::errs(), args...);
}

// Warnings.
template <class... Ts>
inline void fwarn(llvm::raw_ostream &out, Ts const &...args) {
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
inline void fwarnln(llvm::raw_ostream &out, Ts const &...args) {
  fwarn(out, args..., '\n');
}

template <class... Ts>
inline void warnln(Ts const &...args) {
  fwarnln(llvm::errs(), args...);
}

template <class... Ts>
inline void warn(Ts const &...args) {
  fwarn(llvm::errs(), args...);
}

// Verbosity.
template <class... Ts>
inline void finfo(llvm::raw_ostream &out, Ts const &...args) {
  if (VerboseLevel >= Verbosity::quick) {
    fprint(out, args...);
  }
}

template <class... Ts>
inline void finfoln(llvm::raw_ostream &out, Ts const &...args) {
  finfo(out, args..., '\n');
}

template <class... Ts>
inline void infoln(Ts const &...args) {
  finfoln(llvm::errs(), args...);
}

template <class... Ts>
inline void info(Ts const &...args) {
  finfo(llvm::errs(), args...);
}

template <class... Ts>
inline void fdebug(llvm::raw_ostream &out, Ts const &...args) {
  if (VerboseLevel >= Verbosity::detailed) {
    fprint(out, args...);
  }
}

template <class... Ts>
inline void fdebugln(llvm::raw_ostream &out, Ts const &...args) {
  fdebug(out, args..., '\n');
}

template <class... Ts>
inline void debugln(Ts const &...args) {
  fdebugln(llvm::errs(), args...);
}

template <class... Ts>
inline void debug(Ts const &...args) {
  fdebug(llvm::errs(), args...);
}

// Helper to print value names.
inline std::string value_name(const llvm::Value &V) {
  std::string str;
  llvm::raw_string_ostream os(str);
  V.printAsOperand(os, /* print type = */ false);
  // os << ":" << uint64_t(&V);

  return str;
}

inline std::string pretty_use(const llvm::Use &use) {
  std::string str;
  llvm::raw_string_ostream os(str);
  os << "OP " << use.getOperandNo() << " ";
  os << "(" << value_name(*use.get()) << ") ";
  os << "IN " << *use.getUser();
  return str;
}

} // namespace llvm::memoir
#endif
