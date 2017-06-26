#include <iostream>
#include <memory>
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/ADT/StringRef.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [IR-File] [function]" << std::endl;
        return 1;
    }

    llvm::StringRef path = llvm::StringRef(argv[1]);
    llvm::StringRef fn_name = llvm::StringRef(argv[2]);

    // TODO (feliix42): Validate Arguments

    // TODO: Load IR file (multiple?)
    // TODO: Catch errors returned in SMDiagnostic!
    llvm::SMDiagnostic err = llvm::SMDiagnostic();
    llvm::LLVMContext context; // = llvm::LLVMContext();
    std::unique_ptr<llvm::Module> module = llvm::parseIRFile(path, err, context);

    if (!module) {
        std::cout << "Ouch! Couldn't read the IR file:" << std::endl;
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
