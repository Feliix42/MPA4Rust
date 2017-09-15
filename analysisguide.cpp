#include "analysisguide.hpp"

using namespace llvm;

bool isIgnorable(Function* fn) {
    if (!fn->hasName())
    return false;

    std::forward_list<std::string> ignorable {"core::", "_$LT$core..", "alloc::", "_$LT$alloc..", "std::", "_$LT$std.."};

    // demangle the function name
    int s;
    const char* demangled = itaniumDemangle(fn->getName().str().c_str(), nullptr, nullptr, &s);
    if (s != 0) {
        return false;
    }

    // outs() << "(" << demangled << ")";

    std::string fn_name = demangled;
    for (std::string str: ignorable)
        if (fn_name.substr(0, str.size()) == str)
            return true;

    return false;
}


void analyzeFunction(std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* nodelist, Function* fn, std::pair<MessagingNode*, MessagingNode*>* entry_point, const MessageMap* mmap, std::unordered_set<Function*>* visited_fns) {
    // don't check ignorable functions
    if (isIgnorable(fn))
        return;

    // avoid loops
    if (visited_fns->find(fn) != visited_fns->end() && visited_fns->size() > 0)
        return;
    visited_fns->insert(fn);

    // DEBUG
    outs() << "[DEBUG] Checking " << fn->getName() << "\n";

    // initialize containers for BasicBlock check
    std::unordered_set<BasicBlock*> been_there {};
    std::queue<BasicBlock*> unvisited {};

    if (entry_point != nullptr && fn == entry_point->second->instr->getFunction()) {
        if (CallInst* ci = dyn_cast<CallInst>(entry_point->second->instr))
            unvisited.push(ci->getParent());
        else if (InvokeInst* ii = dyn_cast<InvokeInst>(entry_point->second->instr))
            unvisited.push(ii->getSuccessor(0));
    }
    else
        unvisited.push(&fn->getEntryBlock());

    while (unvisited.size() > 0) {
        // get the next BasicBlock
        BasicBlock* cur = unvisited.front();
        unvisited.pop();
        been_there.insert(cur);

        for (Instruction& inst: *cur) {
            if (TerminatorInst* ti = dyn_cast<TerminatorInst>(&inst)) {
                if (InvokeInst* ii = dyn_cast<InvokeInst>(&inst)) {
                    if (isSend(ii)) {
                        for (std::pair<MessagingNode*, MessagingNode*> node_pair: mmap->at(entry_point->second->nspace))
                            // find `send` in message map
                            if (node_pair.first->instr == ii) {
                                // add edge - I don't break here to catch wrongly matched pairings as well.
                                nodelist->push_front(node_pair);
                                // analyze the recv
                                analyzeFunction(nodelist, node_pair.second->instr->getFunction(), &node_pair, mmap, visited_fns);
                            }
                    }
                    else if (ii->getCalledFunction() && ii->getCalledFunction()->getLinkage() > 0) { // TODO: Not sure if this is ok!
                        analyzeFunction(nodelist, ii->getCalledFunction(), entry_point, mmap, visited_fns);
                    }
                }
                else if (SwitchInst* si = dyn_cast<SwitchInst>(&inst)) {
                    // *if* we have an assignment and know what happens to our message, use that knowledge!
                    if (entry_point->first->assignment != -1 && entry_point->second->usage.first == UnwrappedToSwitch) {
                        if (&inst == entry_point->second->usage.second) {
                            // we then only care about the successor the program will take
                            BasicBlock* next = si->getSuccessor(static_cast<unsigned>(entry_point->first->assignment));
                            if (been_there.find(next) == been_there.end())
                                unvisited.push(next);
                            continue;
                        }
                    }
                }

                // for all Terminators, append the successors
                for (BasicBlock* succ: ti->successors()) {
                    if (been_there.find(succ) == been_there.end())
                        unvisited.push(succ);
                }
            }
            else if (CallInst* ci = dyn_cast<CallInst>(&inst)) {
                if (isSend(ci)) {
                    for (std::pair<MessagingNode*, MessagingNode*> node_pair: mmap->at(entry_point->second->nspace))
                        // find `send` in message map
                        if (node_pair.first->instr == ci) {
                            // add edge - I don't break here to catch wrongly matched pairings as well.
                            nodelist->push_front(node_pair);
                            // analyze the recv
                            analyzeFunction(nodelist, node_pair.second->instr->getFunction(), &node_pair, mmap, visited_fns);
                        }
                }
                else if (ci->getCalledFunction() && ci->getCalledFunction()->getLinkage() > 0) {
                    analyzeFunction(nodelist, ci->getCalledFunction(), entry_point, mmap, visited_fns);
                }
            }
        }
    }
}

std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* analyzeGuided(const std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* node_pairs, bool ignore_initial_val) {
    outs() << "[INFO] Starting guided analysis...\n";

    // generate a message map to get a list of message pairs, sorted by the namespace they belong to.
    MessageMap mmap = buildMessageMap(node_pairs);

    // find point to start the analysis
    std::string starting_point = "";
    outs() << "Please specify a starting point for the analysis. You may press enter to display matching components.\n";
    while (true) {
        outs() << "  > ";
        std::getline(std::cin, starting_point);

        if (mmap.find(starting_point) != mmap.end())
            break;
        else {
            outs() << "\nNo exact matches found!\n";
            for (std::pair<std::string, std::forward_list<std::pair<MessagingNode*, MessagingNode*>>> node : mmap)
                if (node.first.find(starting_point) != std::string::npos)
                    outs() << "  " << node.first << "\n";
        }
    }

    // choose a message (content) from the initial sender
    outs() << "Please choose a message dispatch (via line number) to begin.\n";
    std::list<std::pair<unsigned, std::pair<MessagingNode*, MessagingNode*>*>> node_refs {};
    // TODO: Reverse order?
    for (std::pair<MessagingNode*, MessagingNode*> initial_node: mmap[starting_point]) {
        unsigned line = initial_node.first->instr->getDebugLoc()->getLine();

        outs() << "  Line: " << line << " - " << initial_node.first->type << "\n";
        node_refs.push_front(std::make_pair(line, &initial_node));

    }

    std::pair<MessagingNode*, MessagingNode*>* chosen_send = nullptr;
    unsigned chosen_line = 0;
    while (!chosen_send) {
        std::string input;
        outs() << "  > ";
        std::getline(std::cin, input);
        std::stringstream(input) >> chosen_line;

        for (std::pair<unsigned, std::pair<MessagingNode*, MessagingNode*>*> ref: node_refs)
            if (ref.first == chosen_line)
                chosen_send = ref.second;
    }

    outs() << "[DEBUG] Message Properties:\n" \
           << "     > Type: " << chosen_send->first->type << "\n";
    if (chosen_send->first->assignment == -1 || ignore_initial_val) {
        outs() << "     > Instance unknown.\n";

        // let the user chose an instance
        outs() << "Choose an message instance to proceed. ";
        // TODO
    }
    else
        outs() << "     > Instance: " << chosen_send->first->assignment << "\n";

    // let's start!
    std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* nodelist = new std::forward_list<std::pair<MessagingNode*, MessagingNode*>>();
    nodelist->push_front(*chosen_send);

    analyzeFunction(nodelist, chosen_send->second->instr->getFunction(), chosen_send, &mmap, new std::unordered_set<Function*>());

    return nodelist;
}
