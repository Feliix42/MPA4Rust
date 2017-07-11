#include "main.hpp"

using namespace llvm;
namespace fs = ::boost::filesystem;


cl::OptionCategory AnalyzerCategory("Runtime Options", "Options for manipulating the runtime options of the program.");
cl::opt<std::string> IRPath(cl::Positional, cl::desc("<IR file or directory>"), cl::Required);
cl::opt<int> ThreadCount("t", cl::desc("Number of threads to use for calculation"), cl::cat(AnalyzerCategory));
cl::opt<bool> VerboseOutput("v", cl::desc("Turn on verbose mode"));


std::forward_list<std::string> scan_directory(const fs::path& root) {
    std::forward_list<std::string> files = {};
    
    if(!fs::exists(root) || !fs::is_directory(root)) {
        std::cerr << "[ERROR] Path is not a directory but directory traversal was issued." << std::endl;
        return files;
    }

    fs::recursive_directory_iterator it(root);
    fs::recursive_directory_iterator endit;
    
    while(it != endit) {
        if(fs::is_regular_file(*it) && it->path().extension() == ".ll") {
            if (VerboseOutput)
                std::cout << "Loaded: " << it->path().string() << std::endl;
            files.push_front(it->path().string());
        }
        ++it;
    }

    return files;
}


int main(int argc, char** argv) {
    // set up the command line argument parser; hide automatically included options

    cl::HideUnrelatedOptions(AnalyzerCategory);
    cl::ParseCommandLineOptions(argc, argv);

    // TODO (feliix42): Validate Arguments
    if (ThreadCount < 1)
        ThreadCount = 1;

    std::forward_list<std::string> file_list {};
    struct stat s;
    // check if the path is valid and if it describes a file or directory
    // TODO: Might be worth a rewrite using boost
    if (stat(IRPath.c_str(), &s) == 0) {
        if (s.st_mode & S_IFREG) {
            // path specifies valid file
            file_list.push_front(StringRef(IRPath));
        }
        else if (s.st_mode & S_IFDIR) {
            // path specifies a directory
            std::cout << "[INFO] Reading directory." << std::endl;
            file_list = scan_directory(IRPath.c_str());
        }
        else {
            std::cerr << "[ERROR] The path provided does not appear to be a directory, nor a file." << std::endl;
            return 1;
        }
    }
    else {
        std::cerr << "[ERROR] The path provided seems to be invalid." << std::endl;
        return 1;
    }

    // TODO: Do this in threads?

    // TODO: Catch errors returned in SMDiagnostic!
    SMDiagnostic err = SMDiagnostic();
    LLVMContext context;
    std::forward_list<std::unique_ptr<Module>> module_list {};

    std::cout << "[INFO] Loading modules..." << std::endl;
    // TODO: Load all modules in data structure
    //       -> separate structures for threading support?
    for (std::string path: file_list) {
        std::unique_ptr<Module> mod = parseIRFile(StringRef(path), err, context);
        if (!mod) {
            // skip malformed IR Files, emit a note about that.
            std::cerr << "[ERROR] Couldn't read the IR file `" << path << "`. Skipping..." << std::endl;
            err.print("IR File Loader", errs());
        }
        else {
            module_list.push_front(std::move(mod));
        }
    }

    // TODO: Maybe rewrite as LLVM Pass?
    //       Implement scanning (and matching) --> Threading?

    std::cout << "[INFO] Scanning modules for sends/recvs..." << std::endl;

    std::forward_list<MessagingNode> sends, recvs;
    std::tie(sends, recvs) = scan_modules(std::move(module_list), ThreadCount);

    for (MessagingNode m: sends)
        std::cout << "Got send: '" << m.type << "'" << std::endl;
    for (MessagingNode m: recvs)
        std::cout << "Got recv: '" << m.type << "'" << std::endl;
    std::cout << std::endl;

    // start the analysis
    std::cout << "[INFO] Starting Analysis..." << std::endl;
    std::forward_list<std::pair<MessagingNode*, MessagingNode*>> node_pairs = analyzeNodes(sends, recvs);

    for (std::pair<MessagingNode*, MessagingNode*> pair: node_pairs)
        std::cout << "[matched] " << pair.first->type << " --> " << pair.second->type << std::endl;
    return 0;
}
