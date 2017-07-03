#include "scanner.hpp"

using namespace llvm;


std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>> scan_module(std::unique_ptr<Module>& module) {
    std::forward_list<ProgramNode> sends {};
    std::forward_list<ProgramNode> recvs {};

    for (Function& func: module->getFunctionList()) {
        for (BasicBlock& bb: func.getBasicBlockList()) {
            for (Instruction& instr: bb.getInstList()) {
                // Check each instruction. IF it is a function invocation AND a send/recv:
                //    add it to the list, together with all metadata
            }
        }
    }
    
    return std::make_pair(sends, recvs);
}

std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>> scan_modules(std::forward_list<std::unique_ptr<Module>> modules) {
    // do parallelism in this function
    std::forward_list<ProgramNode> sends {}, func_send;
    std::forward_list<ProgramNode> recvs {}, func_recv;
    
    
    for (std::unique_ptr<Module>& mod: modules) {
        std::tie(func_send, func_recv) = scan_module(mod);
        sends.splice_after(sends.cbefore_begin(), func_send);
        recvs.splice_after(recvs.cbefore_begin(), func_recv);
    }
    
    return std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>>::pair();
}

