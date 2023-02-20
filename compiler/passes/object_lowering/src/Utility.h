//
// Created by Peter Zhong on 2/4/23.
//
#include "llvm/Support/raw_ostream.h"
#ifndef OBJECT_LOWERING_UTILITY_H
#define OBJECT_LOWERING_UTILITY_H

#define DEBUG_FUNC llvm::errs()

class Utility {
    public static llvm::raw_ostream &debug() {
        return DEBUG_FUNC;
    }
};


#endif //OBJECT_LOWERING_UTILITY_H
