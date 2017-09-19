#include "properties.hpp"

using namespace llvm;

/**
 Function that tries to determine whether the called instruction is a `send` or not.

 @param demangled_invoke The demangled name of the function that is being called/invoked
 @return `true`, if the function is a send instruction.
 */
bool isSend(std::string demangled_invoke) {
    std::forward_list<std::string> sends {"$LT$std..sync..mpsc..Sender$LT$T$GT$$GT$::send::", \
        "$LT$ipc_channel..ipc..IpcSender$LT$T$GT$$GT$::send::"};

    for (std::string send_instr: sends) {
        unsigned long position = demangled_invoke.find(send_instr);
        if (position != std::string::npos) {
            // sometimes, a closure is called within the send/recv
            // -> to avoid wrong matches, check the suffix of the match for another namespace
            std::string suffix = demangled_invoke.substr(position + send_instr.size(), demangled_invoke.size());
            if (suffix.find("::") == std::string::npos)
                return true;
            else
                return false;
        }
    }

    return false;
}


bool isSend(InvokeInst* ii) {
    if (!ii->getCalledFunction() || !ii->getCalledFunction()->hasName())
        return false;

    // first, demangle the function name
    int s;
    const char* demangled = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
    if (s != 0) {
        return false;
    }

    return isSend(demangled);
}


bool isSend(CallInst* ci) {
    if (!ci->getCalledFunction() || !ci->getCalledFunction()->hasName())
        return false;

    // first, demangle the function name
    int s;
    const char* demangled = itaniumDemangle(ci->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
    if (s != 0) {
        return false;
    }

    return isSend(demangled);
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
    std::forward_list<std::string> recvs {"$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::recv::", \
        "$LT$std..sync..mpsc..Receiver$LT$T$GT$$GT$::try_recv::", \
        "$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::recv::", \
        "$LT$ipc_channel..ipc..IpcReceiver$LT$T$GT$$GT$::try_recv::"};

    for (std::string recv_instr: recvs) {
        unsigned long position = demangled_invoke.find(recv_instr);
        if (position != std::string::npos) {
            std::string suffix = demangled_invoke.substr(position + recv_instr.size(), demangled_invoke.size());
            if (suffix.find("::") == std::string::npos)
                return true;
            else
                return false;
        }
    }

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
std::string getNamespace(const Instruction* inst) {
    // This is just a fallback option.
    // If the compiler output is not optimized, this information is available.
    if (!inst->getDebugLoc())
        return inst->getModule()->getName().str();

    return inst->getDebugLoc()->getFilename().str();
}
