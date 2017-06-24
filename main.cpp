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
    char* fn_name = argv[2];

    // TODO (feliix42): Validate Arguments

    // TODO: Load IR file (multiple?)
    llvm::SMDiagnostic err; // = llvm::SMDiagnostic();
    llvm::LLVMContext context; // = llvm::LLVMContext();
    std::unique_ptr<llvm::Module> module = llvm::parseIRFile(path, err, context);

    // TODO: find entry point
    // TODO: *Maybe* take set of settings (variable bindings etc)?


    return 0;
}
