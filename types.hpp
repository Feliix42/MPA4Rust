#ifndef types_hpp
#define types_hpp

#include <string>
#include <unordered_map>
#include <forward_list>
#include "llvm/IR/Instructions.h"

struct MessagingNode {
    llvm::InvokeInst* instr;
    std::string type;
    std::string nspace;
};

typedef std::unordered_map<std::string, std::forward_list<std::pair<std::string, std::string>>> MessageMap;

#endif /* types_hpp */
