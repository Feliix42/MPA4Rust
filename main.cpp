#include "main.hpp"

using namespace llvm;
namespace fs = ::boost::filesystem;


cl::OptionCategory AnalyzerCategory("Runtime Options", "Options for manipulating the runtime options of the program.");
cl::opt<std::string> IRPath(cl::Positional, cl::desc("<IR file or directory>"), cl::Required);
cl::opt<int> ThreadCount("t", cl::desc("Number of threads to use for calculation"), cl::cat(AnalyzerCategory));
cl::opt<bool> VerboseOutput("v", cl::desc("Turn on verbose mode"), cl::cat(AnalyzerCategory));
cl::opt<std::string> OutputPath("o", cl::desc("Optionally specify an output path for the graph"), cl::cat(AnalyzerCategory), cl::init("message_graph.dot"));


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

    if (ThreadCount < 1)
        ThreadCount = 1;

    if (OutputPath.empty())
        OutputPath = "message_graph.dot";

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

    std::cout << "[INFO] Loading modules..." << std::endl;
    SMDiagnostic err = SMDiagnostic();
    LLVMContext context;
    std::forward_list<std::unique_ptr<Module>> module_list {};

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
    std::tie(sends, recvs) = scan_modules(module_list, ThreadCount);


    // match senders and receivers
    outs() << "[INFO] Starting Analysis...\n";
    std::forward_list<std::pair<MessagingNode*, MessagingNode*>> node_pairs = analyzeNodes(sends, recvs);

    if (VerboseOutput)
        for (std::pair<MessagingNode*, MessagingNode*> pair: node_pairs)
            std::cout << "[matched] " << pair.first->type << " --> " << pair.second->type << std::endl;

    // perform the sender analysis
    for (MessagingNode send: sends) {
        // for further analysis, ignore senders of type "()"
        if (send.type != "()") {
            long long sent_val = analyzeSender(send.instr);
            if (sent_val != -1) {
                outs() << "[Got!] Found assignment of " << sent_val << "\n";
            } else {
                outs() << "[Miss!] Could not find assignment. Type: " << sends.front().type << "\n";
            }
            send.assignment = sent_val;
        }
    }

    // perform the receiver analysis
    for (MessagingNode recv: recvs) {
        // perform the receiver-side analysis
        if (recv.type != "()")
            recv.usage = analyzeReceiver(recv.instr);
    }

    visualize(&node_pairs, OutputPath);
    return 0;
}
