#include "analysis.hpp"

using namespace llvm;

/***************************************** Sender Analysis *****************************************/

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

    // emit information about detected phi nodes for debugging purposes
    if (PHINode* pn = dyn_cast<PHINode>(val))
        // this is just a side note since PHI node encounters are really rare (only one in early tests)
        outs() << "[DEBUG] Found a phi node!\n    " << pn->getModule()->getName() << "\n";

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
            // the found use is either result of a bitcast or used in one
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
    //    outs() << "[WARN] Unhandled value type: " << *val << "\n";

    return nullptr;
}


/**
 Analyze a `send` instruction and try to find out, which value is transmitted over the communication edge.
 This is the central entry point for all sender-analysis matters.

 @param ii The invocation of the `send` instruction.
 @return Returns the assigned constant.
 */
long long analyzeSender(InvokeInst* ii) {
    // start analysis at the value provided as argument to send()
    Value* a = ii->getArgOperand(ii->getNumArgOperands() - 1);
    outs() << "[DEBUG] Identified argument " << *a << "\n";

    std::unordered_set<Value*> been_there {};
    // start the recursive search for an assignment
    StoreInst* si = getRelevantStoreFromValue(a, nullptr, &been_there);

    if (si == nullptr) {
        outs() << "[NOTE] No store instruction found!\n"; // \
        //        << "        Module: " << ii->getModule()->getName() << "\n" \
        //        << "        Instruction: " << *ii << "\n";
        return -1;
    } else {
        if (ConstantInt* assigned_number = dyn_cast<ConstantInt>(si->getValueOperand())) {
            outs() << "[SUCCESS] Found a viable assignment: " << assigned_number->getValue() << " is assigned.\n";
            return assigned_number->getValue().getSExtValue();
        } else {
            errs() << "[ERR] Could only find: " << *si << "\n";  // TODO: Can be removed since this is checked in the function now
            return -1;
        }
    }
}



/***************************************** Receiver Analysis *****************************************/

bool isResultUnwrap(InvokeInst* ii) {
    if (!ii->getCalledFunction() || !ii->getCalledFunction()->hasName())
        return false;

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
 Recursively iterates over the value produced by the receive instruction and the subsequent values
 produced by bitcasts, load and store instructions, etc.

 This function aims to find possible value unwraps (as every (try_)recv() returns a Result type) and
 the concrete use of the value in a control flow decision (such as match statements and function
 invocations. It collects these possible matches in an unordered map provided as function argument.

 @param val The value to inspect, initially an InvokeInst.
 @param been_there A set used to detect loops and avoid re-visiting nodes.
 @param possible_matches This map collects the possible matches we will inspect later on.
 */
void analyzeReceiveInst(Value* val, std::unordered_set<Value*>* been_there, std::unordered_map<BasicBlock*, Instruction*>* possible_matches) {
    if (been_there->find(val) == been_there->end())
        been_there->insert(val);
    else
        return;

    // follow the instruction types
    // identify and handle unwrap operations!

    // the current value is an invoke instruction (e.g. a `receive` call or an unwrap etc)
    if (InvokeInst* ii = dyn_cast<InvokeInst>(val)) {
        if (ii->getCalledFunction()) {
            // first, demangle the function name to be able to check what function we are looking at
            int s;
            const char* demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
            if (s == 0 && isRecv(demangled_name) && ii->hasStructRetAttr())
                // if the instruction is the receive (this is always true for the instruction we start with), follow the return
                    analyzeReceiveInst(ii->getArgOperand(0), been_there, possible_matches);
        }
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
            //            outs() << "[INFO] Found unwrap/relevant switch block.\n";
        }
        else if (InvokeInst* ii = dyn_cast<InvokeInst>(u)) {
            if (isResultUnwrap(ii)) {
                //                outs() << "    -> marked current node as IOI.\n";
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
                //                outs() << "[INFO] Possible message handler detected: " << *ii << "\n";
                //                outs() << "       -> marked current node as IOI.\n";
            }

        }
    }
    // TODO: Add case for Optionals(?) and received result types, i.e. Receiver<Result<bool>>(?)
}


/**
 This function takes the map of possible matches and the basic block after the recv() call and
 then traverses the control flow graph in order to match the instructions from the possible_matches
 set to the control flow graph and find the value unwraps and message usages.

 @param bb The initial basic block to begin the search from.
 @param possible_matches The map of possible matches generated from the `analyzeReceiveInst` function
 @param valueUnwrapped Tracks wether the received value has been unwrapped yet.
 @param last_hit Tracks the last matched value fro mthe possible matches.
 @return The usage type and the corresponding instruction.
 */
std::pair<UsageType, Instruction*>* findUsageInstruction(BasicBlock* bb, std::unordered_set<BasicBlock*> path_history, std::unordered_map<BasicBlock*, Instruction*>* possible_matches, bool valueUnwrapped, Instruction* last_hit) {
    outs() << "  Now checking: " << bb->getName() << "\n";
    // stop when no instructions to check are left or we are in a loop
    if (possible_matches->size() == 0 || path_history.find(bb) != path_history.end()) {
        outs() << "[WARN] All matches have been checked or detected loop.\n";
        if (valueUnwrapped)
            return new std::pair<UsageType, Instruction*>(UnwrappedDirectUse, last_hit);
        else
            return new std::pair<UsageType, Instruction*>(DirectUse, nullptr);
    }

    // mark the node as "visited on the current path"
    path_history.insert(bb);

    // TODO: Can Basic Blocks have 0 successors?
    if (BasicBlock* next_bb = bb->getSingleSuccessor()) {
        // if a basic block only has a single successor we can skip it as the relevant
        // BBs have 2+ successors (due to the nature of switch/invoke instructions)
        outs() << "    -> skipping to next BB\n";
        outs() << "       (" << bb->getParent()->getName() << ")\n";
        return findUsageInstruction(next_bb, path_history, possible_matches, valueUnwrapped, last_hit);
    }
    else {
        // we have to handle multiple successors
        if (possible_matches->find(bb) != possible_matches->end()) {
            // the basic block contains a possible match
            Instruction* inst = possible_matches->at(bb);
            if (SwitchInst* si = dyn_cast<SwitchInst>(inst)) {
                //                outs() << "[INFO] Found a switch instruction.";
                if (valueUnwrapped) {
                    //                    outs() << " Value already unwrapped. Done.\n" << *inst << "\n";
                    return new std::pair<UsageType, Instruction*>(UnwrappedToSwitch, inst);
                }
                else {
                    //                    outs() << " Is value unwrap. \n" << *inst << "\n";
                    possible_matches->erase(bb);
                    return findUsageInstruction(si->getSuccessor(1), path_history, possible_matches, true, inst);
                }
            }
            else if (InvokeInst* ii = dyn_cast<InvokeInst>(inst)) {
                //                outs() << "[INFO] Found a function invocation.";
                if (valueUnwrapped) {
                    //                    outs() << " Value already unwrapped. Done.\n" << *inst << "\n";
                    return new std::pair<UsageType, Instruction*>(UnwrappedToHandlerFunction, inst);
                }
                else {
                    if (isResultUnwrap(ii)) {
                        //                        outs() << " Is value unwrap. \n" << *ii << "\n";
                        possible_matches->erase(bb);
                        return findUsageInstruction(ii->getSuccessor(0), path_history, possible_matches, true, inst);
                    }
                    else {
                        //                        outs() << " Is direct handler function call.\n" << *ii << "\n";
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
                std::pair<UsageType, Instruction*>* tmp_result = findUsageInstruction(next_bb, path_history, possible_matches, valueUnwrapped, last_hit);
                if (tmp_result->first >= result->first)
                    result = tmp_result;
            }
            return result;
        }
    }
    return nullptr;
}


/**
 Analyze the `receive` instruction, find the instruction where the received value is used.
 This is the wrapper around all receiver-analysis logic and can be called directly, providing
 the receive() call in question.

 @param ii The invocation of the recv() function.
 */
std::pair<UsageType, Instruction*>* analyzeReceiver(InvokeInst* ii) {
    // initialize the data structures for the analysis
    std::unordered_set<Value*> been_there {};
    // this map will contain all possible unwrap/handle operations which will be sorted out later
    std::unordered_map<BasicBlock*, Instruction*> possible_matches {};

    outs() << "[INFO] Analyzing receive Instruction...\n" \
    << "       " << *ii << "\n";

    analyzeReceiveInst(ii, &been_there, &possible_matches);

    outs() << "[INFO] Finding the relevant usage...\n";

    // now sort out the previously filtered instructions by traversing the Control Flow Graph,
    // starting at the first Basic block after the `receive` function was called

    std::pair<UsageType, Instruction*>* usage = findUsageInstruction(ii->getSuccessor(0), {}, &possible_matches, false, nullptr);

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

    return usage;
}
