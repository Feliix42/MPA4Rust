#ifndef properties_hpp
#define properties_hpp

#include <string>
#include <forward_list>

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
//Debug Information and Metadata
#include "llvm/IR/DebugInfoMetadata.h"

// name demangling
#include "llvm/Demangle/Demangle.h"

#include "types.hpp"

bool isSend(std::string demangled_invoke);
bool isSend(llvm::InvokeInst* ii);
bool isSend(llvm::CallInst* ci);
const char* getSentType(std::string struct_name);

bool isRecv(std::string demangled_invoke);
const char* getReceivedType(std::string struct_name);

std::string getNamespace(const llvm::Instruction* ii);

#endif /* properties_hpp */
