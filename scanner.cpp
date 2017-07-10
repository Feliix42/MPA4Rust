#include "scanner.hpp"

using namespace llvm;


bool isSend(std::string demangled_invoke) {
    std::forward_list<std::string> sends {"$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$::send", \
        "$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$::send"};
    bool found = false;
    for (std::string send_instr: sends) {
        found = found || demangled_invoke.find(send_instr) != std::string::npos;
    }
    return found;
}


bool isRecv(std::string demangled_invoke) {
    std::forward_list<std::string> recvs {"$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::recv", \
        "$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::try_recv", \
        "$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::recv", \
        "$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::try_recv"};
    bool found = false;
    for (std::string recv_instr: recvs) {
        found = found || demangled_invoke.find(recv_instr) != std::string::npos;
    }
    return found;
}


std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>> scan_module(std::unique_ptr<Module>& module) {
    std::forward_list<MessagingNode> sends {};
    std::forward_list<MessagingNode> recvs {};

    // Iterate through all functions through all basic blocks over every instruction within the module
    for (Function& func: module->getFunctionList()) {
        for (BasicBlock& bb: func.getBasicBlockList()) {
            for (Instruction& instr: bb.getInstList()) {
                if (InvokeInst* ii = dyn_cast<InvokeInst>(&instr)) {

                    // check if it's an direct function invocation that has a name
                    if (ii->getCalledFunction() && ii->getCalledFunction()->hasName()) {
                        // compare the demangled function name to find out whether it's a send or recv
                        int s;
                        const char* demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
                        if (s != 0)
                            break;
                        if (isSend(demangled_name)) {
                            /* Debug Section */
                            std::cout << std::endl;
                            std::cout << "[INFO] Current Module: " << ii->getModule()->getName().str() << std::endl;
                            std::cout << "[INFO] Hit `send` in function " << ii->getFunction()->getName().str() << std::endl;
                            std::cout << "  Called Function: " << ii->getCalledFunction()->getName().str() << std::endl;
                            if (demangled_name)
                                std::cout << "  Demangled: " << demangled_name << std::endl;

                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                // check if argument is a PointerType (which is the case for the Sender/Receiver structs.
                                if (isa<PointerType>(arg.getType()))
                                    std::cout << dyn_cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
                                else
                                    arg.getType()->print(outs());

                                std::cout << "  -- " << arg.getArgNo() << std::endl;
                            }
                            std::cout << std::endl;


                            /* Where the magic will happen */
                            sends.push_front(MessagingNode {ii, ""});
                        }
                        else if (isRecv(demangled_name)) {
                            /* Debug Section */
                            std::cout << "[INFO] Current Module: " << ii->getModule()->getName().str() << std::endl;
                            std::cout << "[INFO] Hit `recv` in function " << ii->getFunction()->getName().str() << std::endl;
                            std::cout << "  Called Function: " << ii->getCalledFunction()->getName().str() << std::endl;
                            std::cout << "  Demangled: " << demangled_name << std::endl;

                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                // check if argument is a PointerType (which is the case for the Sender/Receiver structs.
                                if (isa<PointerType>(arg.getType()))
                                    std::cout << dyn_cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
                                else
                                    arg.getType()->print(outs());

                                std::cout << "  -- " << arg.getArgNo() << std::endl;
                            }
                            std::cout << std::endl;

                            /* Where the magic will happen */
                            recvs.push_front(MessagingNode {ii, ""});
                        }
                        // TODO: Difference between recv and try_recv?
                    }
                }
            }
        }

        // TODO: maybe make this an opt-in thingy via CLI option?
        // func.viewCFG();
    }

    return std::make_pair(sends, recvs);
}

std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>> scan_modules(std::forward_list<std::unique_ptr<Module>> modules, int thread_no) {
    // TODO: do parallelism in this function
    std::forward_list<MessagingNode> sends {}, func_send;
    std::forward_list<MessagingNode> recvs {}, func_recv;

    

    for (std::unique_ptr<Module>& mod: modules) {
        std::tie(func_send, func_recv) = scan_module(mod);
        sends.splice_after(sends.cbefore_begin(), func_send);
        recvs.splice_after(recvs.cbefore_begin(), func_recv);
    }

    return std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>>::pair();
}

