#ifndef scanner_hpp
#define scanner_hpp

#include <tuple>
#include <forward_list>

// just for io stuff
#include <iostream>
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

// name demangling
#include "llvm/Demangle/Demangle.h"


struct ProgramNode {
    int wow;
};

std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>> scan_modules(std::forward_list<std::unique_ptr<llvm::Module>> modules);

#endif /* scanner_hpp */
