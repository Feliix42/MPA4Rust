#ifndef scanner_hpp
#define scanner_hpp

#include <tuple>
#include <forward_list>
#include <unordered_set>
#include <stack>
#include <string>

// just for io stuff
#include <iostream>
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/Operator.h"

// MemoryTransfer Instructions (like memcpy etc)
#include "llvm/IR/IntrinsicInst.h"

// name demangling
#include "llvm/Demangle/Demangle.h"

#include "types.hpp"
#include "properties.hpp"


// function definitions
std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>> scan_modules(std::forward_list<std::unique_ptr<llvm::Module>>& modules, int thread_no);

#endif /* scanner_hpp */
