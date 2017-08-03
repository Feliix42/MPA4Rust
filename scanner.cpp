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
//    const Function* f = func;
//    std::string name = "";
//
//    // TODO: Problems: Does not consider the function name itself atm.
//    //          -> implement this to e.g. check for main function
//    //          -> reliably find {{closure}} call (a.k.a. thread::spawn and thread::Builder::spawn
//    //          -> multiple calls?
//
//    bool cont = true;
//    int status;
//    const char* function_name = itaniumDemangle(f->getName().str().c_str(), nullptr, nullptr, &status);
//    if (status != 0)
//        std::cout << "[ERROR] Couldn't demangle name of the function that contains the send." << std::endl;
//    else {
//        std::cout << "[DEBUG] Found in iteration 0: " << function_name << std::endl;
//        std::cout << "[DEBUG] Subprogram name: " << f->getSubprogram()->getName().str() << std::endl;
//        std::cout << "[DEBUG] Scope: " << f->getSubprogram()->getScope().resolve()->getName().str() << std::endl;
//    }
//
//    int i = 1;
//    while (cont) {
//        bool new_found = false;
//        std::cout << "  [DEBUG] # users: " << f->getNumUses() << std::endl;
//        for (const User* u: f->users()) {
//            const char* demangled_name;
//            int s = -1;
//
//            if (const InvokeInst* ii = dyn_cast<InvokeInst>(u)) {
//                demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
//                f = ii->getFunction();
//                new_found = true;
//            }
//            else if (const CallInst* ci = dyn_cast<CallInst>(u)) {
//                demangled_name = itaniumDemangle(ci->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
//                f = ci->getFunction();
//                new_found = true;
//            }
//            else {
//                std::cerr << "ouch" << std::endl;
//                u->dump();
//            }
//
//
//            if (s != 0)
//                continue;
//            else {
//                std::cout << "[DEBUG] Found in iteration " << i << ": " << demangled_name << std::endl;
//                name = f->getSubprogram()->getName().str();
//                std::cout << "[DEBUG] Subprogram name: " << name << std::endl;
//                std::cout << "[DEBUG] Scope: " << f->getSubprogram()->getScope().resolve()->getName().str() << std::endl;
//            }
//        }
//
//        //quit searching when no new users have been found
//        if (!new_found)
//            break;
//
//        if (name == "{{closure}}")
//            cont = false;
//
//        i++;
//    }

    // TODO: I'm not yet satisfied with the way this function works.
    // It seems that some parts of the namespace are replaced with an {{impl}}
    // instead of the name of the struct the function is implemented for.
    // Check, whether this can be "adjusted". -- Feliix42 (2017-07-17)
//    std::stack<std::string> namestack;
//
//    DIScope* sc = func->getSubprogram()->getScope().resolve();
//    namestack.push(sc->getName().str());
//
//    std::cout << "tree: " << sc->getName().str();
//    while (sc->getScope().resolve()) {
//        sc = sc->getScope().resolve();
//        std::cout << " - " << sc->getName().str();
//        namestack.push(sc->getName().str());
//    }
//    std::cout << std::endl;
//
//    // build the namespace string, append "::" between the separate namespace identifiers
//    std::string ns = namestack.top();
//    namestack.pop();
//    while (namestack.size() > 0 && namestack.top() != "{{impl}}") {
//        ns.append("::").append(namestack.top());
//        namestack.pop();
//    }
//
//    std::cout << "converted: " << ns << std::endl;
//    return ns;

    return func->getParent()->getName().str();
}


StoreInst* getRelevantStoreFromValue(Value* val, Value* prev, std::unordered_set<Value*>* been_there) {
    // this set keeps book about the statements we have seen so far to detect loops
    if (been_there->find(val) == been_there->end())
        been_there->insert(val);
    else
        return nullptr;

//    errs() << "Inspect: " << *val << "\n" << val->getNumUses() << " uses:\n";
//    for (User* u: val->users()) {
//        errs() << *u << "\n";
//    }


    // hacky workaround:
    if (BitCastInst* bi = dyn_cast<BitCastInst>(val)) {
        StoreInst* deep_store_inst = getRelevantStoreFromValue(bi->getOperand(0), val, been_there);
        if (deep_store_inst != nullptr)
            return deep_store_inst;
    }
    for (User* u: val->users()) {
        if (LoadInst* li = dyn_cast<LoadInst>(u)) {
            StoreInst* deep_store_inst = getRelevantStoreFromValue(li->getPointerOperand(), val, been_there);
            if (deep_store_inst != nullptr)
                return deep_store_inst;
        }
        if (StoreInst* si = dyn_cast<StoreInst>(u)) {
            // we are done after having found a constant assignment
            if (isa<ConstantInt>(si->getValueOperand()))
                return si;
            else {
                StoreInst* deep_store_inst = getRelevantStoreFromValue(si->getValueOperand(), val, been_there);
                if (deep_store_inst != nullptr)
                    return deep_store_inst;
            }
        }
        if (BitCastInst* bi = dyn_cast<BitCastInst>(u)) {
            StoreInst* deep_store_inst;
            if (bi->getOperand(0) == val)
                deep_store_inst = getRelevantStoreFromValue(bi, val, been_there);
            else
                deep_store_inst = getRelevantStoreFromValue(bi->getOperand(0), val, been_there);

            if (deep_store_inst != nullptr)
                return deep_store_inst;
        }
        if (AllocaInst* ai = dyn_cast<AllocaInst>(u))
            continue;
        if (MemTransferInst* mi = dyn_cast<MemTransferInst>(u)) {
            StoreInst* deep_store_inst;
            if (mi->getRawDest() == val)
                deep_store_inst = getRelevantStoreFromValue(mi->getRawSource(), val, been_there);
            else
                deep_store_inst = getRelevantStoreFromValue(mi->getRawDest(), val, been_there);

            if (deep_store_inst != nullptr)
                return deep_store_inst;
        }
        if (GetElementPtrInst* gi = dyn_cast<GetElementPtrInst>(u)) {
            StoreInst* deep_store_inst = getRelevantStoreFromValue(gi, val, been_there);
            if (deep_store_inst != nullptr)
                return deep_store_inst;
        }
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
                            // unfortunately, some instructions that match here are no "real" sends so we have
                            // to filter them out by trying to find the sender struct, since these "false sends" don't have one
                            bool real_send = false;
                            for (Argument& arg: ii->getCalledFunction()->getArgumentList()) {
                                if (isa<PointerType>(arg.getType())) {
                                    std::string struct_name = cast<PointerType>(arg.getType())->getElementType()->getStructName().str();
                                    if (const char* sent_type = getSentType(std::move(struct_name))) {
                                        // save the send instruction for matching later on
                                        sends.push_front(MessagingNode {ii, sent_type, getNamespace(&func)});
                                        real_send = true;
                                    }
                                }
                            }

                            // if a "real" send has been found, try to get more information about the transmitted value
                            // TODO: only check the sent values if type != ()
                            if (real_send && sends.front().type != "()") {
                                std::cout << "Found a 'real' send in instruction:" << std::endl;

                                ii->dump();
                                Value* a = ii->getArgOperand(ii->getNumArgOperands() - 1);
                                errs() << "Identified argument " << *a << "\n";

                                std::cout << "[DEBUG] Getting relevant store instruction(s)" << std::endl;
                                std::unordered_set<Value*> been_there {};
                                StoreInst* si = getRelevantStoreFromValue(a, nullptr, &been_there);
                                if (si == nullptr)
                                    std::cerr << "[ERROR] No store instruction found!" << std::endl;
                                else {
                                    if (ConstantInt* assigned_number = dyn_cast<ConstantInt>(si->getValueOperand()))
                                        outs() << "[INFO] Found a viable assignment: " << assigned_number->getValue() << " is assigned.\n";
                                    else
                                        errs() << "[ERR] Could only find: " << *si << "\n";
//                                    std::cout << " -- " << si->getPointerOperand()->isUsedByMetadata() << std::endl;
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

