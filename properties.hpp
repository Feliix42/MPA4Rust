#ifndef properties_hpp
#define properties_hpp

#include <string>
#include <forward_list>

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
//Debug Information and Metadata
#include "llvm/IR/DebugInfoMetadata.h"

bool isSend(std::string demangled_invoke);
const char* getSentType(std::string struct_name);

bool isRecv(std::string demangled_invoke);
const char* getReceivedType(std::string struct_name);
std::string getNamespace(const llvm::InvokeInst* ii);

#endif /* properties_hpp */
