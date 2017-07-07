#include "scanner.hpp"

using namespace llvm;


bool isSend(std::string demangled_invoke) {
    std::forward_list<std::string> sends {"$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$::send"};
    bool found = false;
    for (std::string send_instr: sends) {
        found = found || demangled_invoke.find(send_instr) != std::string::npos;
    }
    return found;
}


bool isRecv(std::string demangled_invoke) {
    std::forward_list<std::string> recvs {"$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::recv", "$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::try_recv"};
    bool found = false;
    for (std::string recv_instr: recvs) {
        found = found || demangled_invoke.find(recv_instr) != std::string::npos;
    }
    return found;
}


std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>> scan_module(std::unique_ptr<Module>& module) {
    std::forward_list<ProgramNode> sends {};
    std::forward_list<ProgramNode> recvs {};

    // Iterate through all functions through all basic blocks over every instruction within the module
//    std::cout << "[INFO] Checking module " << module->getName().str() << std::endl;
    for (Function& func: module->getFunctionList()) {
//        std::cout << std::endl << "[INFO] Checking function " << func.getName().str() << std::endl;
        for (BasicBlock& bb: func.getBasicBlockList()) {
            for (Instruction& instr: bb.getInstList()) {
                if (InvokeInst* ii = dyn_cast<InvokeInst>(&instr)) {
//                    ii->print(outs());
//                    std::cout << std::endl;

                    // check if it's an direct function invocation that has a name
                    if (ii->getCalledFunction() && ii->getCalledFunction()->hasName()) {
                        // compare the demangled function name to find out whether it's a send or recv
                        std::string demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, nullptr);
                        if (isSend(demangled_name)) {
                            std::cout << "[INFO] Current Module: " << ii->getModule()->getName().str() << std::endl;
                            std::cout << "[INFO] Hit `send` in function " << ii->getFunction()->getName().str() << std::endl;
                            std::cout << "  Called Function: " << ii->getCalledFunction()->getName().str() << std::endl;
                            std::cout << "  Demangled: " << demangled_name << std::endl;

                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                arg.print(outs());
                                std::cout << std::endl;
                            }
                            std::cout << std::endl;
                        }
                        else if (isRecv(demangled_name)) {
                            std::cout << "[INFO] Current Module: " << ii->getModule()->getName().str() << std::endl;
                            std::cout << "[INFO] Hit `recv` in function " << ii->getFunction()->getName().str() << std::endl;
                            std::cout << "  Called Function: " << ii->getCalledFunction()->getName().str() << std::endl;
                            std::cout << "  Demangled: " << demangled_name << std::endl;

                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                arg.print(outs());
                                std::cout << std::endl;
                            }
                            std::cout << std::endl;
                        }
                        // TODO: Difference between recv and try_recv?
                        // TODO: Check IPC-channels
                    }

//                    std::cout << std::endl << std::endl;
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
    // TODO: do parallelism in this function
    std::forward_list<ProgramNode> sends {}, func_send;
    std::forward_list<ProgramNode> recvs {}, func_recv;


    for (std::unique_ptr<Module>& mod: modules) {
        std::tie(func_send, func_recv) = scan_module(mod);
        sends.splice_after(sends.cbefore_begin(), func_send);
        recvs.splice_after(recvs.cbefore_begin(), func_recv);
    }

    return std::pair<std::forward_list<ProgramNode>, std::forward_list<ProgramNode>>::pair();
}

