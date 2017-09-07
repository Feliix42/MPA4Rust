#ifndef analysis_hpp
#define analysis_hpp

#include <unordered_set>

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


long long analyzeSender(llvm::InvokeInst* ii);
void analyzeReceiver(llvm::InvokeInst* ii);

#endif /* analysis_hpp */
