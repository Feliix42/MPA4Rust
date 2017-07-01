#include <forward_list>
#include <iostream>
#include <memory>
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include <sys/stat.h>


std::forward_list<llvm::StringRef> scan_directory(std::string path) {
    return std::forward_list<llvm::StringRef>::forward_list();
}


int main(int argc, char** argv) {
    // parse command line arguments
    llvm::cl::opt<std::string> IRPath(llvm::cl::Positional, llvm::cl::desc("<IR file or directory>"), llvm::cl::Required);
    llvm::cl::ParseCommandLineOptions(argc, argv);
    
    // TODO (feliix42): Validate Arguments
    
    // check if the path is valid and if it describes a file or directory
    std::forward_list<llvm::StringRef> file_list {};
    struct stat s;
    if (stat(IRPath.c_str(), &s) == 0) {
        if (s.st_mode & S_IFREG) {
            // path specifies valid file
            file_list.push_front(llvm::StringRef(IRPath));
        } else if (s.st_mode & S_IFDIR) {
            // path specifies a directory
            file_list = scan_directory(IRPath);
            // TODO: Implement!
        } else {
            std::cout << "[ERROR] The path provided does not appear to be a directory, nor a file." << std::endl;
            return 1;
        }
    } else {
        std::cout << "[ERROR] The path provided seems to be invalid." << std::endl;
        return 1;
    }
    

    // TODO: Load IR file (multiple?)
    // TODO: Catch errors returned in SMDiagnostic!
    llvm::SMDiagnostic err = llvm::SMDiagnostic();
    llvm::LLVMContext context; // = llvm::LLVMContext();
    
    // TODO: Load all modules in data structure
    std::unique_ptr<llvm::Module> module = llvm::parseIRFile(path, err, context);

    if (!module) {
        std::cout << "[ERROR] Ouch! Couldn't read the IR file." << std::endl;
        // TODO: do some more error handling ( == printing)
        // skip malformed IR Files, emit a note about that.
        return 1;
    }


    return 0;
}
