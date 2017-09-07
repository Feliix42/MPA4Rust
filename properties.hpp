#ifndef properties_hpp
#define properties_hpp

#include <string>
#include <forward_list>

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

bool isSend(std::string demangled_invoke);
const char* getSentType(std::string struct_name);

bool isRecv(std::string demangled_invoke);
const char* getReceivedType(std::string struct_name);
std::string getNamespace(const llvm::Function* func);

#endif /* properties_hpp */
