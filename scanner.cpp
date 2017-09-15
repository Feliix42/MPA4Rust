#include "scanner.hpp"

using namespace llvm;


std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>> scan_module(std::unique_ptr<Module>& module) {
    std::forward_list<MessagingNode> sends {};
    std::forward_list<MessagingNode> recvs {};

    // Iterate through all functions through all basic blocks over every instruction within the modules
    for (Function& func: module->getFunctionList())
        for (BasicBlock& bb: func.getBasicBlockList()) {
            // check the terminator of the basic block (could be a `send` invocation)
            if (InvokeInst* ii = dyn_cast<InvokeInst>(bb.getTerminator()))
                // check if it's an direct function invocation that has a name
                if (ii->getCalledFunction() && ii->getCalledFunction()->hasName()) {
                    // compare the demangled function name to find out whether it's a send or recv
                    int s;
                    const char* demangled_name = itaniumDemangle(ii->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
                    if (s != 0)
                        continue;

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
                            sends.push_front(MessagingNode {ii, sent_type, getNamespace(ii), .assignment = -1});
                        else
                            continue;
                    }
                    else if (isRecv(demangled_name)) {
                        // functionality similar to the branch above
                        std::string struct_name;
                        if (ii->hasStructRetAttr())
                            struct_name = cast<PointerType>(ii->getArgOperand(1)->getType())->getElementType()->getStructName().str();
                        else
                            struct_name = cast<PointerType>(ii->getArgOperand(0)->getType())->getElementType()->getStructName().str();

                        if (const char* recv_type = getReceivedType(std::move(struct_name)))
                            recvs.push_front(MessagingNode {ii, recv_type, getNamespace(ii), .usage = std::make_pair(Unchecked, (Instruction*) nullptr)});
                        else
                            continue;
                    }
                }
            for (Instruction& inst: bb.getInstList()) {
                if (CallInst* ci = dyn_cast<CallInst>(&inst)) {
                    if (!ci->getCalledFunction() || !ci->getCalledFunction()->hasName())
                        continue;
                    // compare the demangled function name to find out whether it's a send or recv
                    int s;
                    const char* demangled_name = itaniumDemangle(ci->getCalledFunction()->getName().str().c_str(), nullptr, nullptr, &s);
                    if (s != 0)
                        continue;

                    // Instruction *is* sending something
                    if (isSend(demangled_name)) {
                        // outs() << "[GOT] a send!\n";
                        // the argument to check is determined by whether the first argument is the return value or not
                        std::string struct_name;
                        if (ci->hasStructRetAttr())
                            struct_name = cast<PointerType>(ci->getArgOperand(1)->getType())->getElementType()->getStructName().str();
                        else
                            struct_name = cast<PointerType>(ci->getArgOperand(0)->getType())->getElementType()->getStructName().str();

                        // ignore any failures when extracting the types by simply skipping the value
                        if (const char* sent_type = getSentType(std::move(struct_name)))
                            sends.push_front(MessagingNode {ci, sent_type, getNamespace(ci), .assignment = -1});
                        else
                            continue;
                    }
                    else if (isRecv(demangled_name)) {
                        // outs() << "[GOT] a recv!\n";
                        // functionality similar to the branch above
                        std::string struct_name;
                        if (ci->hasStructRetAttr())
                            struct_name = cast<PointerType>(ci->getArgOperand(1)->getType())->getElementType()->getStructName().str();
                        else
                            struct_name = cast<PointerType>(ci->getArgOperand(0)->getType())->getElementType()->getStructName().str();

                        if (const char* recv_type = getReceivedType(std::move(struct_name)))
                            recvs.push_front(MessagingNode {ci, recv_type, getNamespace(ci), .usage = std::make_pair(Unchecked, (Instruction*) nullptr)});
                        else
                            continue;
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

