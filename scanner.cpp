#include "scanner.hpp"

using namespace llvm;


std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>> scan_module(std::unique_ptr<Module>& module) {
    std::forward_list<ProgramNode> sends {};
    std::forward_list<ProgramNode> recvs {};

    std::cout << "[INFO] Checking module " << module->getName().str() << std::endl;
    for (Function& func: module->getFunctionList()) {
        std::cout << std::endl << "[INFO] Checking function " << func.getName().str() << std::endl;
        for (BasicBlock& bb: func.getBasicBlockList()) {
            for (Instruction& instr: bb.getInstList()) {
                if (InvokeInst* ii = dyn_cast<InvokeInst>(&instr)) {
                    /* Begin Debugging Output Section */
                    ii->print(outs());
                    std::cout << std::endl;
                    
                    if (ii->getCalledFunction()) {
                        if (ii->getCalledFunction()->hasName()) {
                            std:: cout << "  Called Function: " << ii->getCalledFunction()->getName().str() << std::endl;
                            std::cout << "  Demangled: " << itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, nullptr) << std::endl;
                        }
                        for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                            arg.print(outs());
                            std::cout << std::endl;
                        }
                    }
                    else
                        std::cout << "[NOTE] Indirect function invokation." << std::endl;
                    
                    std::cout << std::endl << std::endl;
                    /* End Debugging Output Section */
                }
            }
        }
        
        // TODO: maybe make this an opt-in thingy via CLI option?
        // func.viewCFG();
    }
    std::cout << std::endl << std::endl;
    
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

