#ifndef types_hpp
#define types_hpp

#include <string>
#include <unordered_map>
#include <forward_list>
#include "llvm/IR/Instructions.h"

enum UsageType {
    Unchecked,                  ///< This node was not checked.
    DirectUse,                  ///< The received value is being used directly in the function, no unwrap involved.
    DirectHandlerCall,          ///< The value is passed directly into a handler function.
    UnwrappedDirectUse,         ///< The received value is unwrapped and used directly.
    UnwrappedToHandlerFunction, ///< The unwrapped value is used as argument to a handler function.
    UnwrappedToSwitch           ///< The unwrapped value is used in a switch statement.
};

struct MessagingNode {
    llvm::Instruction* instr;
    std::string type;
    std::string nspace;
    union {
        long long assignment;
        std::pair<UsageType, llvm::Instruction*> usage;
    };

};

typedef std::unordered_map<std::string, std::forward_list<std::pair<MessagingNode*, MessagingNode*>>> MessageMap;



#endif /* types_hpp */
