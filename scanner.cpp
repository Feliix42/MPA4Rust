#include "scanner.hpp"

using namespace llvm;



/**
 Function that tries to determine whether the called instruction is a `send` or not.

 @param demangled_invoke The demangled name of the function that is being called/invoked
 @return `true`, if the function is a send instruction.
 */
bool isSend(std::string demangled_invoke) {
    std::forward_list<std::string> sends {"$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$::send", \
        "$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$::send"};

    for (std::string send_instr: sends)
        if (demangled_invoke.find(send_instr) != std::string::npos)
            return true;

    return false;
}


/**
 Function that extracts the type of the message that is being sent over the channel
 by inspecting the type of the sender.
 
 Normally, a sender in Rusts LLVM IR is defined as follows:
        std::sync::mpsc::Sender<T>

 This function extracts the transmitted type T, if one exists.

 @param struct_name Type name of the sender struct
 @return The name of the sent type, or a `nullptr`.
 */
const char* getSentType(std::string struct_name) {
    std::forward_list<std::string> senders {"std::sync::mpsc::Sender<", \
        "ipc_channel::ipc::IpcSender<"};

    // see if the string starts with "std::sync::mpsc::Sender<" (or the IPC equivalent)
    // and extract the type thet is being sent
    for (std::string single_sender: senders) {
        if (struct_name.find(single_sender) != std::string::npos) {
            struct_name.erase(0, single_sender.length());
            struct_name.erase(struct_name.rfind(">"), struct_name.length());
            return struct_name.c_str();
        }
    }

    return nullptr;
}


/**
 Function that tries to determine whether the called instruction is a `recv` or not.

 @param demangled_invoke The demangled name of the function that is being called/invoked
 @return `true`, if the function is a receive instruction.
 */
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


/**
 Function that extracts the type of the message that is being received over the channel
 by inspecting the type of the receiver.

 Normally, a receiver in Rusts LLVM IR is defined as follows:
 std::sync::mpsc::Receiver<T>

 This function extracts the transmitted type T, if one exists.

 @param struct_name Type name of the receiver struct
 @return The name of the received type, or a `nullptr`.
 */
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


// TODO: This function has to be reworked
//  --> get the users of the function that contains the send
//      --> trace the tree back up until you find a thread (spawn)
//  --> return the thread names (?) or sth like that as namespace(s)
//  --> return a list!
// TODO: Find cross-module stuff
inline std::string getNamespace(const Function* func) {
    // TODO: I'm not yet satisfied with the way this function works.
    // It seems that some parts of the namespace are replaced with an {{impl}}
    // instead of the name of the struct the function is implemented for.
    // Check, whether this can be "adjusted". -- Feliix42 (2017-07-17)
    std::stack<std::string> namestack;

    DIScope* sc = func->getSubprogram()->getScope().resolve();
    namestack.push(sc->getName().str());

    std::cout << "tree: " << sc->getName().str();
    while (sc->getScope().resolve()) {
        sc = sc->getScope().resolve();
        std::cout << " - " << sc->getName().str();
        namestack.push(sc->getName().str());
    }
    std::cout << std::endl;

    // build the namespace string, append "::" between the separate namespace identifiers
    std::string ns = namestack.top();
    namestack.pop();
    while (namestack.size() > 0 && namestack.top() != "{{impl}}") {
        ns.append("::").append(namestack.top());
        namestack.pop();
    }

    std::cout << "converted: " << ns << std::endl;
    return ns;
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
                            // unfortunately, some instructions that match here are no "real" sends so we have
                            // to filter them out by trying to find the sender struct, since these "false sends" don't have one
                            bool real_send = false;
                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                if (isa<PointerType>(arg.getType())) {
                                    std::string struct_name = cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
                                    if (const char* type = getSentType(std::move(struct_name))) {
                                        // save the send instruction for matching later on
                                        sends.push_front(MessagingNode {ii, type, getNamespace(&func)});
                                        real_send = true;
                                    }
                                }
                            }

                            // if a "real" send has been found, try to get more information about the transmitted value
                            if (real_send) {
                                std::cout << "Found a 'real' send." << std::endl;

                                ii->dump();
                                Value* a = ii->getArgOperand(ii->getNumArgOperands() - 1);
                                a->dump();
                                if (LoadInst* li = dyn_cast<LoadInst>(a)) {
                                    std::cout << "Load Instruction" << std::endl;
                                    li->getPointerOperand()->dump();
                                }
                            }
                        }
                        else if (isRecv(demangled_name)) {
                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                if (isa<PointerType>(arg.getType())) {
                                    std::string struct_name = cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
                                    if (const char* type = getReceivedType(std::move(struct_name))) {
                                        // save the receive instruction for matching later on
                                        recvs.push_front(MessagingNode {ii, type, getNamespace(&func)});
                                    }
                                }
                            }
                        }
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

