#include <iostream>
#include <memory>
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"

int main(int argc, char** argv) {
    // parse command line arguments
    llvm::cl::ParseCommandLineOptions(argc, argv);
    llvm::cl::opt<std::string> IRFile(llvm::cl::Positional, llvm::cl::desc("<IR file>"), llvm::cl::Required);
    llvm::cl::opt<std::string> EntryFunction(llvm::cl::Positional, llvm::cl::desc("<entry point (function name)>"), llvm::cl::Required);
    
    // check if required options are present
    if( IRFile.length() == 0 || EntryFunction.length() == 0) {
        llvm::cl::PrintHelpMessage();
        return 1;
    }
    
    // convert options
    llvm::StringRef path = llvm::StringRef(IRFile);
    llvm::StringRef fn_name = llvm::StringRef(EntryFunction);

    
    // TODO (feliix42): Validate Arguments

    // TODO: Load IR file (multiple?)
    // TODO: Catch errors returned in SMDiagnostic!
    llvm::SMDiagnostic err = llvm::SMDiagnostic();
    llvm::LLVMContext context; // = llvm::LLVMContext();
    std::unique_ptr<llvm::Module> module = llvm::parseIRFile(path, err, context);

    if (!module) {
        std::cout << "[ERROR] Ouch! Couldn't read the IR file." << std::endl;
        // TODO: do some more error handling ( == printing)
        return 1;
    }


    // TODO: find entry point
    // verify the entry point exists
    llvm::Function* entry_point = module->getFunction(fn_name);
    if (!entry_point) {
        std::cout << "[ERROR] Specified function `" << fn_name.str() << "`not found. Exiting..." << std::endl;
    }

    // TODO: *Maybe* take set of settings (variable bindings etc)?


    return 0;
}
