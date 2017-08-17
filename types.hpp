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


enum UsageType {
    DirectUse,                  ///< The received value is being used directly in the function, no unwrap involved.
    DirectHandlerCall,          ///< The value is passed directly into a handler function.
    UnwrappedDirectUse,         ///< The received value is unwrapped and used directly.
    UnwrappedToHandlerFunction, ///< The unwrapped value is used as argument to a handler function.
    UnwrappedToSwitch           ///< The unwrapped value is used in a switch statement.
};

#endif /* types_hpp */
