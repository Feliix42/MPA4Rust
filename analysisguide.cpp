#include "analysisguide.hpp"

using namespace llvm;

bool isIgnorable(Function* fn) {
    if (!fn->hasName())
    return false;

    std::forward_list<std::string> ignorable {"core::", "_$LT$core..", "alloc::", "_$LT$alloc.."}; //, "std::", "_$LT$std.."};

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


void analyzeFunction(std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* nodelist, Function* fn, std::pair<MessagingNode*, MessagingNode*> entry_point, const MessageMap* mmap, std::unordered_set<Function*>* visited_fns) {
//     don't check ignorable functions
//    if (isIgnorable(fn))
//        return;

    if (fn->getSubprogram())
        outs() << "Now checking: " << fn->getSubprogram()->getName() << "\n";
    else
        outs() << "Now checking: " << fn->getName() << "\n";

    // avoid loops
    if (visited_fns->find(fn) != visited_fns->end() && visited_fns->size() > 0)
        return;
    visited_fns->insert(fn);

    // DEBUG
//    outs() << "[DEBUG] Checking " << fn->getName() << "\n";

    // initialize containers for BasicBlock check
    std::unordered_set<BasicBlock*> been_there {};
    std::queue<BasicBlock*> unvisited {};

    // if we have no entry point, we receive a pair of nullptrs

    if (entry_point.first != nullptr && fn == entry_point.second->instr->getFunction()) {
        if (CallInst* ci = dyn_cast<CallInst>(entry_point.second->instr))
            unvisited.push(ci->getParent());
        else if (InvokeInst* ii = dyn_cast<InvokeInst>(entry_point.second->instr))
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
                        if (ii->getCalledFunction()->getSubprogram())
                            outs() << "send " << ii->getCalledFunction()->getSubprogram()->getName();
                        else
                            outs() << "send " << ii->getCalledFunction()->getName();

                        if (entry_point.first != nullptr) {
                            for (std::pair<MessagingNode*, MessagingNode*> node_pair: mmap->at(entry_point.second->nspace))
                                // find `send` in message map
                                if (node_pair.first->instr == ii) {
                                    outs() << " got hit!";
                                    // add edge - I don't break here to catch wrongly matched pairings as well.
                                    nodelist->push_front(node_pair);
                                    // analyze the recv
                                    analyzeFunction(nodelist, node_pair.second->instr->getFunction(), node_pair, mmap, visited_fns);
                                }
                        }
                        else {
                            // we have no entry point -> generate it using the invoke instruction
                            for (std::pair<MessagingNode*, MessagingNode*> node_pair: mmap->at(getNamespace(ii)))
                                // find `send` in message map
                                if (node_pair.first->instr == ii) {
                                    outs() << " got hit!";
                                    // add edge - I don't break here to catch wrongly matched pairings as well.
                                    nodelist->push_front(node_pair);
                                    // analyze the recv
                                    analyzeFunction(nodelist, node_pair.second->instr->getFunction(), node_pair, mmap, visited_fns);
                                }
                        }
                        outs() << "\n";
                    }
                    else if (ii->getCalledFunction()) { // && ii->getCalledFunction()->getLinkage() > 0) { // TODO: Not sure if this is ok!
                        if (ii->getCalledFunction()->getSubprogram())
                            outs() << "Analyze " << ii->getCalledFunction()->getSubprogram()->getName() << "\n";
                        else
                            outs() << "Analyze " << ii->getCalledFunction()->getName() << "\n";
                        analyzeFunction(nodelist, ii->getCalledFunction(), entry_point, mmap, visited_fns);
                    }
                }
                else if (SwitchInst* si = dyn_cast<SwitchInst>(&inst)) {
                    // *if* we have an assignment and know what happens to our message, use that knowledge!
                    if (entry_point.first !=nullptr && entry_point.first->assignment != -1 && entry_point.second->usage.first == UnwrappedToSwitch) {
                        if (&inst == entry_point.second->usage.second) {
                            // we then only care about the successor the program will take
                            BasicBlock* next = si->getSuccessor(static_cast<unsigned>(entry_point.first->assignment));
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
                    if (ci->getCalledFunction()->getSubprogram())
                        outs() << "send call " << ci->getCalledFunction()->getSubprogram()->getName();
                    else
                        outs() << "send call " << ci->getCalledFunction()->getName();
                    if (entry_point.first != nullptr) {
                        for (std::pair<MessagingNode*, MessagingNode*> node_pair: mmap->at(entry_point.second->nspace))
                            // find `send` in message map
                            if (node_pair.first->instr == ci) {
                                // add edge - I don't break here to catch wrongly matched pairings as well.
                                nodelist->push_front(node_pair);
                                // analyze the recv
                                outs() << "pair " << &node_pair << "\n";
                                analyzeFunction(nodelist, node_pair.second->instr->getFunction(), node_pair, mmap, visited_fns);
                            }
                    }
                    else {
                        for (std::pair<MessagingNode*, MessagingNode*> node_pair: mmap->at(getNamespace(ci)))
                            // find `send` in message map
                            if (node_pair.first->instr == ci) {
                                outs() << " got hit!";
                                // add edge - I don't break here to catch wrongly matched pairings as well.
                                nodelist->push_front(node_pair);
                                // analyze the recv
                                analyzeFunction(nodelist, node_pair.second->instr->getFunction(), node_pair, mmap, visited_fns);
                            }
                    }
                    outs() << "\n";
                }
                else if (ci->getCalledFunction()) { // && ci->getCalledFunction()->getLinkage() > 0) {
                    if (ci->getCalledFunction()->getSubprogram())
                        outs() << "Analyze " << ci->getCalledFunction()->getSubprogram()->getName() << "\n";
                    else
                        outs() << "Analyze " << ci->getCalledFunction()->getName() << "\n";
                    analyzeFunction(nodelist, ci->getCalledFunction(), entry_point, mmap, visited_fns);
                }
            }
        }
    }
}


std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* analyzeGuidedFromFunction(MessageMap mmap, std::string module_name) {
    outs() << "Please select a function to start (Only sending functions are shown).\n";

    // print function names available
    std::unordered_map<std::string, Function*> function_map {};
    for (std::pair<MessagingNode*, MessagingNode*> node: mmap[module_name]) {
        std::string func_name = node.first->instr->getFunction()->getSubprogram()->getName();
//        outs() << "  " << func_name << "\n";
        function_map[func_name] = node.first->instr->getFunction();
    }

    for (std::pair<std::string, Function*> fn: function_map)
        outs() << "  " << fn.first << "\n";

    std::string chosen_func = "";
    while (true) {
        outs() << "  > ";
        std::getline(std::cin, chosen_func);

        if (function_map.find(chosen_func) != function_map.end())
            break;
    }

    std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* nodelist = new std::forward_list<std::pair<MessagingNode*, MessagingNode*>>();

    analyzeFunction(nodelist, function_map[chosen_func], std::make_pair(nullptr, nullptr), &mmap, new std::unordered_set<Function*>());

    return nodelist;
}


std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* analyzeGuided(const std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* node_pairs, bool ignore_initial_val, bool choose_function) {
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

    // switch to a different function for this analysis
    if (choose_function) {
        return analyzeGuidedFromFunction(std::move(mmap), starting_point);
    }

    // choose a message (content) from the initial sender
    outs() << "Please choose a message dispatch (via line number) to begin.\n";
    std::forward_list<std::pair<unsigned, std::pair<MessagingNode*, MessagingNode*>>> node_refs {};
    for (std::pair<MessagingNode*, MessagingNode*> initial_node: mmap[starting_point]) {
        unsigned line = initial_node.first->instr->getDebugLoc()->getLine();

        outs() << "  Line: " << line << " - " << initial_node.first->type << "\n";
//        outs() << "        > " << &initial_node << "\n";
        node_refs.push_front(std::make_pair(line, initial_node));
    }

    std::pair<MessagingNode*, MessagingNode*> chosen_send;
    unsigned chosen_line = 0;
    bool done = false;
    while (!done) {
        std::string input;
        outs() << "  > ";
        std::getline(std::cin, input);
        std::stringstream(input) >> chosen_line;

        outs() << "input: " << chosen_line << "\n";

        for (std::pair<unsigned, std::pair<MessagingNode*, MessagingNode*>> ref: node_refs) {
//            outs() << "Checking line " << ref.first << "\n";
            if (ref.first == chosen_line) {
                outs() << "HIT: " << chosen_line << "\n";
//                outs() << "Assigning " << ref.second << "\n";
                chosen_send = ref.second;
                done = true;
                break;
            }
        }
    }

    outs() << "Name: " << chosen_send.first->instr->getFunction()->getSubprogram()->getName() << "\n";

    outs() << "[DEBUG] Message Properties:\n" \
           << "     > Type: " << chosen_send.first->type << "\n" \
//           << "     > Pair: " << chosen_send << "\n"
           << "     > Line: " << chosen_send.first->instr->getDebugLoc()->getLine() << "\n";
    if (chosen_send.first->assignment == -1 || ignore_initial_val) {
        outs() << "     > Instance unknown.\n";

        // let the user chose an instance
        outs() << "Choose an message instance to proceed. ";
        // TODO
    }
    else
        outs() << "     > Instance: " << chosen_send.first->assignment << "\n";

    // let's start!
    std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* nodelist = new std::forward_list<std::pair<MessagingNode*, MessagingNode*>>();
    nodelist->push_front(chosen_send);

    analyzeFunction(nodelist, chosen_send.second->instr->getFunction(), chosen_send, &mmap, new std::unordered_set<Function*>());

    return nodelist;
}
