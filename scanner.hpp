#ifndef scanner_hpp
#define scanner_hpp

#include <tuple>
#include <forward_list>

#include "llvm/IR/Module.h"


struct ProgramNode {
    int wow;
};

std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>> scan_modules(std::forward_list<std::unique_ptr<llvm::Module>> modules);

#endif /* scanner_hpp */
