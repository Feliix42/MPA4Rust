#include "scanner.hpp"

using namespace llvm;


bool isSend(std::string demangled_invoke) {
    std::forward_list<std::string> sends {"$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$::send", \
        "$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$::send"};

    for (std::string send_instr: sends)
        if (demangled_invoke.find(send_instr) != std::string::npos)
            return true;

    return false;
}


const char* getSentType(std::string struct_name) {
    // see if string starts with "std::sync::mpsc::Sender<" (or the IPC equivalent)
    // and extract the type
    //  IDEA: Substring matches for the Sender identifier and the trailing >
    //      -> Cut these parts out, return type
    std::forward_list<std::string> senders {"std::sync::mpsc::Sender<", \
        "ipc_channel::ipc::IpcSender<"};

    for (std::string single_sender: senders) {
        if (struct_name.find(single_sender) != std::string::npos) {
            struct_name.erase(0, single_sender.length());
            struct_name.erase(struct_name.rfind(">"), struct_name.length());
            return struct_name.c_str();
        }
    }

    return nullptr;
}


bool isRecv(std::string demangled_invoke) {
    std::forward_list<std::string> recvs {"$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::recv", \
        "$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::try_recv", \
        "$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::recv", \
        "$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::try_recv"};

    for (std::string recv_instr: recvs)
        if (demangled_invoke.find(recv_instr) != std::string::npos)
            return true;

    return false;
}


// TODO: check, if received type is correctly identified for IpcSenders!!!
const char* getReceivedType(std::string struct_name) {
    std::forward_list<std::string> receivers {"std::sync::mpsc::Receiver<", \
        "ipc_channel::ipc::IpcReceiver<"};

    for (std::string single_receiver: receivers)
        if (struct_name.find(single_receiver) != std::string::npos) {
            struct_name.erase(0, single_receiver.length());
            struct_name.erase(struct_name.rfind(">"), struct_name.length());
            return struct_name.c_str();
        }

    return nullptr;
}


std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>> scan_module(std::unique_ptr<Module>& module) {
    std::forward_list<MessagingNode> sends {};
    std::forward_list<MessagingNode> recvs {};

    // Iterate through all functions through all basic blocks over every instruction within the modules
    for (Function& func: module->getFunctionList())
        // TODO: maybe make this an opt-in thingy via CLI option?
        // func.viewCFG();
        for (BasicBlock& bb: func.getBasicBlockList())
            for (Instruction& instr: bb.getInstList())
                if (InvokeInst* ii = dyn_cast<InvokeInst>(&instr))
                    // check if it's an direct function invocation that has a name
                    if (ii->getCalledFunction() && ii->getCalledFunction()->hasName()) {
                        // compare the demangled function name to find out whether it's a send or recv
                        int s;
                        const char* demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
                        if (s != 0)
                            break;

                        // Instruction *is* sending something
                        if (isSend(demangled_name)) {
                            /* Debug Section */
//                            std::cout << std::endl;
//                            std::cout << "[INFO] Current Module: " << ii->getModule()->getName().str() << std::endl;
//                            std::cout << "[INFO] Hit `send` in function " << ii->getFunction()->getName().str() << std::endl;
//                            if (ii->getFunction()->getSectionPrefix().hasValue())
//                                std::cout << "[INFO] Section prefix: " << ii->getFunction()->getSectionPrefix().getValue().str() << std::endl;
//                            std::cout << "  Called Function: " << ii->getCalledFunction()->getName().str() << std::endl;
//                            if (demangled_name)
//                                std::cout << "  Demangled: " << demangled_name << std::endl;
//
//                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
//                                // check if argument is a PointerType (which is the case for the Sender/Receiver structs.
//                                if (isa<PointerType>(arg.getType()))
//                                    std::cout << dyn_cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
//                                else
//                                    arg.getType()->print(outs());
//
//                                std::cout << "  -- " << arg.getArgNo() << std::endl;
//                            }
//                            std::cout << std::endl;
                            /* Where the magic will happen */

                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                if (isa<PointerType>(arg.getType())) {
                                    std::string struct_name = cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
                                    if (const char* type = getSentType(std::move(struct_name))) {
                                        // std::cout << "Extracted: " << type << std::endl;
                                        sends.push_front(MessagingNode {ii, type});
                                    }
                                }
                            }
                        }
                        else if (isRecv(demangled_name)) {
                            /* Debug Section */
//                            std::cout << "[INFO] Current Module: " << ii->getModule()->getName().str() << std::endl;
//                            std::cout << "[INFO] Hit `recv` in function " << ii->getFunction()->getName().str() << std::endl;
//                            if (ii->getFunction()->getSectionPrefix().hasValue())
//                                std::cout << "[INFO] Section prefix: " << ii->getFunction()->getSectionPrefix().getValue().str() << std::endl;
//                            std::cout << "  Called Function: " << ii->getCalledFunction()->getName().str() << std::endl;
//                            std::cout << "  Demangled: " << demangled_name << std::endl;
//
//                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
//                                // check if argument is a PointerType (which is the case for the Sender/Receiver structs.
//                                if (isa<PointerType>(arg.getType()))
//                                    std::cout << dyn_cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
//                                else
//                                    arg.getType()->print(outs());
//
//                                std::cout << "  -- " << arg.getArgNo() << std::endl;
//                            }
//                            std::cout << std::endl;

                            /* Where the magic will happen */

                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                if (isa<PointerType>(arg.getType())) {
                                    std::string struct_name = cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
                                    if (const char* type = getReceivedType(std::move(struct_name))) {
                                        std::cout << "Extracted: " << type << std::endl;
                                        recvs.push_front(MessagingNode {ii, type});
                                    }
                                }
                            }
                        }
                        // TODO: Difference between recv and try_recv?
                    }


    return std::make_pair(sends, recvs);
}

std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>> scan_modules(std::forward_list<std::unique_ptr<Module>>& modules, int thread_no) {
    // TODO: do parallelism in this function
    std::forward_list<MessagingNode> sends {}, func_send;
    std::forward_list<MessagingNode> recvs {}, func_recv;

    

    for (std::unique_ptr<Module>& mod: modules) {
        std::tie(func_send, func_recv) = scan_module(mod);
        sends.splice_after(sends.cbefore_begin(), func_send);
        recvs.splice_after(recvs.cbefore_begin(), func_recv);
    }

    return std::make_pair(sends, recvs);
}

