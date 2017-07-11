#ifndef types_hpp
#define types_hpp

#include <string>
#include "llvm/IR/Instructions.h"

struct MessagingNode {
    llvm::InvokeInst* instr;
    std::string type;
};

#endif /* types_hpp */
