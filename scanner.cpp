#include "scanner.hpp"

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


bool isUnwrap(InvokeInst* ii) {
    // first, demangle the function name
    int s;
    const char* demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
    if (s != 0) {
        errs() << "[ERROR] Failed to demangle function name of " << ii->getName() << "\n";
        return false;
    }

    std::string demangled_invoke = demangled_name;
    unsigned long position = demangled_invoke.find("$LT$core..result..Result$LT$T$C$$u20$E$GT$$GT$::unwrap::");
    if (position != std::string::npos)
        return true;
    else
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

/**
 Things to improve:
    - do we need the `prev` argument?
    - recognition and handling of PHI nodes -> there would be only a single immediate use case for it
    - recognition and handling of "unwrap" operations (unwrapping result and option types) -> necessary?
    - there are some operations we are currently not equipped to handle
    - trace senders that send optionals/results?
    - get some structure into the function
    - how to save the values?
 */
StoreInst* getRelevantStoreFromValue(Value* val, Value* prev, std::unordered_set<Value*>* been_there) {
    // this set keeps book about the statements we have seen so far to detect loops
    if (been_there->find(val) == been_there->end())
        been_there->insert(val);
    else
        return nullptr;

//    outs() << "Inspect: " << *val << "\n" << val->getNumUses() << " uses:\n";
//    for (User* u: val->users()) {
//        outs() << *u << "\n";
//    }
//    outs() << "\n";


    // hacky workarounds:
    // emit information about detected phi nodes for debugging purposes
    if (PHINode* pn = dyn_cast<PHINode>(val))
        outs() << "[PHI] Found a phi node!\n    " << pn->getModule()->getName() << "\n";

    if (BitCastInst* bi = dyn_cast<BitCastInst>(val)) {
        StoreInst* deep_store_inst = getRelevantStoreFromValue(bi->getOperand(0), val, been_there);
        if (deep_store_inst != nullptr)
            return deep_store_inst;
    }
    if (LoadInst* li = dyn_cast<LoadInst>(val)) {
        StoreInst* deep_store_inst = getRelevantStoreFromValue(li->getPointerOperand(), val, been_there);
        if (deep_store_inst != nullptr)
            return deep_store_inst;
    }

    for (User* u: val->users()) {
        if (LoadInst* li = dyn_cast<LoadInst>(u)) {
//            li->dump();
            StoreInst* deep_store_inst = getRelevantStoreFromValue(li->getPointerOperand(), val, been_there);
            if (deep_store_inst != nullptr)
                return deep_store_inst;
        }
        if (StoreInst* si = dyn_cast<StoreInst>(u)) {
//            si->dump();
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
//            bi->dump();
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
//            mi->dump();
//            // begin debug
//            errs() << "Raw Source: " << *mi->getRawSource() << "\n";
//            errs() << "Raw Dest: " << *mi->getRawDest() << "\n";
//            // end

            StoreInst* deep_store_inst;
            if (mi->getRawDest() == val)
                deep_store_inst = getRelevantStoreFromValue(mi->getRawSource(), val, been_there);
            else
                deep_store_inst = getRelevantStoreFromValue(mi->getRawDest(), val, been_there);

            if (deep_store_inst != nullptr)
                return deep_store_inst;
        }
        if (GetElementPtrInst* gi = dyn_cast<GetElementPtrInst>(u)) {
//            gi->dump();
            StoreInst* deep_store_inst = getRelevantStoreFromValue(gi, val, been_there);
            if (deep_store_inst != nullptr)
                return deep_store_inst;
        }
    }

//    outs() << "[WARN] Unhandled value type: " << *val << "\n";

    return nullptr;
}


void analyzeReceiveInst(Value* val, std::unordered_set<Value*>* been_there, std::unordered_map<BasicBlock*, Instruction*>* possible_matches) {
    if (been_there->find(val) == been_there->end())
        been_there->insert(val);
    else
        return;

    // follow the instruction types
    // identify and handle unwrap operations!
    outs() << "[A-RECV] Current value: " << *val << "\n" \
           << "    -> Users: \n";
    for (User* u: val->users())
        outs() << "        " << *u << "\n";

    // the current value is an invoke instruction (e.g. a `receive` call or an unwrap etc)
    if (InvokeInst* ii = dyn_cast<InvokeInst>(val)) {
        // first, demangle the function name to be able to check what function we are looking at
        int s;
        const char* demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
        if (s != 0)
            errs() << "[ERROR] Failed to demangle function name of " << ii->getName() << "\n";

        if (isRecv(demangled_name))
            // if the instruction is the receive (this is always true for the instruction we start with), follow the return value
            if (ii->hasStructRetAttr())
                analyzeReceiveInst(ii->getArgOperand(0), been_there, possible_matches);
    }
    else if (BitCastInst* bi = dyn_cast<BitCastInst>(val))
        analyzeReceiveInst(bi->getOperand(0), been_there, possible_matches);

    for (User* u: val->users()) {
        if (StoreInst* si = dyn_cast<StoreInst>(u)) {
            if (si->getPointerOperand() != val) // TODO: this might be redundant?
                 analyzeReceiveInst(si->getPointerOperand(), been_there, possible_matches);
            else
                analyzeReceiveInst(si->getValueOperand(), been_there, possible_matches);
        }
        else if (LoadInst* li = dyn_cast<LoadInst>(u)) {
            analyzeReceiveInst(li, been_there, possible_matches);
            if (li->getPointerOperand() != val)
                analyzeReceiveInst(li->getPointerOperand(), been_there, possible_matches);
        }
        else if (BitCastInst* bi = dyn_cast<BitCastInst>(u)) {
            analyzeReceiveInst(bi, been_there, possible_matches);
            if (bi->getOperand(0) != val)
                analyzeReceiveInst(bi->getOperand(0), been_there, possible_matches);
        }
        else if (MemTransferInst* mi = dyn_cast<MemTransferInst>(u)) {
            analyzeReceiveInst(mi->getRawDest(), been_there, possible_matches);
            if (mi->getRawSource() != val)
                analyzeReceiveInst(mi->getRawSource(), been_there, possible_matches);
        }
        else if (GetElementPtrInst* gi = dyn_cast<GetElementPtrInst>(u)) {
            analyzeReceiveInst(gi, been_there, possible_matches);
        }
        else if (ZExtInst* zi = dyn_cast<ZExtInst>(u)) {
            analyzeReceiveInst(zi, been_there, possible_matches);
        }
        else if (SwitchInst* si = dyn_cast<SwitchInst>(u)) {
            possible_matches->insert({si->getParent(), si});
            outs() << "[DONE] Found unwrap/relevant switch block.\n";
        }
        else if (InvokeInst* ii = dyn_cast<InvokeInst>(u)) {
            if (isUnwrap(ii)) {
                outs() << "    -> marked current node as IOI.\n";
                possible_matches->insert({ii->getParent(), ii});
                if (ii->hasStructRetAttr()) {
                    been_there->insert(ii);
                    analyzeReceiveInst(ii->getArgOperand(0), been_there, possible_matches);
                }
                else
                    analyzeReceiveInst(ii, been_there, possible_matches);
            }
            else if (been_there->find(ii) == been_there->end()){
                been_there->insert(ii);
                possible_matches->insert({ii->getParent(), ii});
                outs() << "[INFO] Possible message handler detected: " << *ii << "\n";
                outs() << "       -> marked current node as IOI.\n";
            }

        }
    }
    // TODO: Add case for Optionals(?) and received result types, i.e. Receiver<Result<bool>>(?)
}


std::pair<UsageType, Instruction*>* findUsageInstruction(BasicBlock* bb, std::unordered_map<BasicBlock*, Instruction*>* possible_matches, bool valueUnwrapped, Instruction* last_hit) {
    outs() << "  Now checking: " << bb->getName() << "\n";
    // stop when no instructions to check are left
    if (possible_matches->size() == 0) {
        outs() << "[WARN] All matches have been checked.\n";
        if (valueUnwrapped)
            return new std::pair<UsageType, Instruction*>(UnwrappedDirectUse, last_hit);
        else
            return new std::pair<UsageType, Instruction*>(DirectUse, nullptr);
    }

    // TODO: Can Basic Blocks have 0 successors?
    if (BasicBlock* next_bb = bb->getSingleSuccessor()) {
        // if a basic block only has a single successor we can skip it as the relevant
        // BBs have 2+ successors (due to the nature of switch/invoke instructions)
        outs() << "    -> skipping to next BB\n";
        return findUsageInstruction(next_bb, possible_matches, valueUnwrapped, last_hit);
    }
    else {
        // we have to handle multiple successors
        if (possible_matches->find(bb) != possible_matches->end()) {
            // the basic block contains a possible match
            Instruction* inst = possible_matches->at(bb);
            if (SwitchInst* si = dyn_cast<SwitchInst>(inst)) {
                outs() << "[INFO] Found a switch instruction.";
                if (valueUnwrapped) {
                    outs() << " Value already unwrapped. Done.\n" << *inst << "\n";
                    return new std::pair<UsageType, Instruction*>(UnwrappedToSwitch, inst);
                }
                else {
                    outs() << " Is value unwrap. \n" << *inst << "\n";
                    possible_matches->erase(bb);
                    return findUsageInstruction(si->getSuccessor(1), possible_matches, true, inst);
                }
            }
            else if (InvokeInst* ii = dyn_cast<InvokeInst>(inst)) {
                outs() << "[INFO] Found a function invocation.";
                if (valueUnwrapped) {
                    outs() << " Value already unwrapped. Done.\n" << *inst << "\n";
                    return new std::pair<UsageType, Instruction*>(UnwrappedToHandlerFunction, inst);
                }
                else {
                    if (isUnwrap(ii)) {
                        outs() << " Is value unwrap. \n" << *ii << "\n";
                        possible_matches->erase(bb);
                        return findUsageInstruction(ii->getSuccessor(0), possible_matches, true, inst);
                    }
                    else {
                        outs() << " Is direct handler function call.\n" << *ii << "\n";
                        possible_matches->erase(bb);
                        return new std::pair<UsageType, Instruction*>(DirectHandlerCall, inst);
                    }

                }
            }
        }
        else {
            // check every successor of the current basic block as we do not know what causes the split here
            std::pair<UsageType, Instruction*>* result = new std::pair<UsageType, Instruction*>(DirectUse, nullptr);
            for (BasicBlock* next_bb: bb->getTerminator()->successors()) {
                std::pair<UsageType, Instruction*>* tmp_result = findUsageInstruction(next_bb, possible_matches, valueUnwrapped, last_hit);
                if (tmp_result->first >= result->first)
                    result = tmp_result;
            }
            return result;
        }
    }
    return nullptr;
}


void analyzeReceiver(InvokeInst* ii) {
    // initialize the data structures for the analysis
    std::unordered_set<Value*> been_there {};
    // this map will contain all possible unwrap/handle operations which will be sorted out later
    std::unordered_map<BasicBlock*, Instruction*> possible_matches {};

    outs() << "[INFO] Analyzing receive Instruction...\n";

    analyzeReceiveInst(ii, &been_there, &possible_matches);

    outs() << "[INFO] Finding the relevant usage...\n";

    // now sort out the previously filtered instructions by traversing the Control Flow Graph,
    // starting at the first Basic block after the `receive` function was called
    std::pair<UsageType, Instruction*>* usage = findUsageInstruction(ii->getSuccessor(0), &possible_matches, false, nullptr);

    switch (usage->first) {
        case DirectUse:
            outs() << "[INFO] Received value is used directly, no control-flow branching detected.\n";
            break;
        case DirectHandlerCall:
            outs() << "[INFO] Received value is directly passed to handler function:\n" << *usage->second << "\n";
            break;
        case UnwrappedDirectUse:
            outs() << "[INFO] Received value is unwrapped und used without further control-flow branching.\n";
            break;
        case UnwrappedToHandlerFunction:
            outs() << "[INFO] Received value is unwrapped and passed to handler function:\n" << *usage->second << "\n";
            break;
        case UnwrappedToSwitch:
            outs() << "[INFO] Received value is unwrapped, control-flow branches here:\n" << *usage->second << "\n";
            break;
    }
}


std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>> scan_module(std::unique_ptr<Module>& module) {
    std::forward_list<MessagingNode> sends {};
    std::forward_list<MessagingNode> recvs {};

    // Iterate through all functions through all basic blocks over every instruction within the modules
    for (Function& func: module->getFunctionList())
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
                            // the argument to check is determined by whether the first argument is the return value or not
                            std::string struct_name;
                            if (ii->hasStructRetAttr())
                                struct_name = cast<PointerType>(ii->getArgOperand(1)->getType())->getElementType()->getStructName().str();
                            else
                                struct_name = cast<PointerType>(ii->getArgOperand(0)->getType())->getElementType()->getStructName().str();

                            // ignore any failures when extracting the types by simply skipping the value
                            if (const char* sent_type = getSentType(std::move(struct_name)))
                                sends.push_front(MessagingNode {ii, sent_type, getNamespace(&func)});
                            else
                                continue;

                            // for further analysis, ignore senders of type "()"
                            if (sends.front().type != "()") {
                                Value* a = ii->getArgOperand(ii->getNumArgOperands() - 1);
                                outs() << "[DEBUG] Identified argument " << *a << "\n";

                                std::unordered_set<Value*> been_there {};
                                StoreInst* si = getRelevantStoreFromValue(a, nullptr, &been_there);
                                if (si == nullptr)
                                    outs() << "[ERROR] No store instruction found!\n" \
                                           << "        Module: " << ii->getModule()->getName() << "\n" \
                                           << "        Instruction: " << *ii << "\n";
                                else {
                                    if (ConstantInt* assigned_number = dyn_cast<ConstantInt>(si->getValueOperand()))
                                        outs() << "[SUCCESS] Found a viable assignment: " << assigned_number->getValue() << " is assigned.\n";
                                    else
                                        errs() << "[ERR] Could only find: " << *si << "\n";  // TODO: Can be removed since this is checked in the function now
                                }
                            }
                        }
                        else if (isRecv(demangled_name)) {
                            // functionality similar to the branch above
                            std::string struct_name;
                            if (ii->hasStructRetAttr())
                                struct_name = cast<PointerType>(ii->getArgOperand(1)->getType())->getElementType()->getStructName().str();
                            else
                                struct_name = cast<PointerType>(ii->getArgOperand(0)->getType())->getElementType()->getStructName().str();

                            if (const char* recv_type = getReceivedType(std::move(struct_name)))
                                recvs.push_front(MessagingNode {ii, recv_type, getNamespace(&func)});
                            else
                                continue;

                            // perform the receiver-side analysis
                            if (recvs.front().type != "()") {
                                analyzeReceiver(ii);
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

