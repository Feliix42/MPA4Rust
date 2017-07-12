#include "visualizer.hpp"

MessageMap buildMessageMap(std::forward_list<std::pair<MessagingNode, MessagingNode>> node_pairs) {
    MessageMap mmap = MessageMap();
    for (std::pair<MessagingNode, MessagingNode> pair: node_pairs) {
        std::string sender_module = pair.first.instr->getModule()->getName().str();
        std::string receiver_module = pair.second.instr->getModule()->getName().str();

        mmap[sender_module].push_front(std::make_pair(receiver_module, pair.first.type));
        if (mmap.find(receiver_module) == mmap.end())
            mmap.insert(std::make_pair(receiver_module, std::forward_list<std::pair<std::string, std::string>>::forward_list()));
    }

    return mmap;
}


std::string getNodeName (std::string verbose_name) {
    std::hash<std::string> hash_fn;
    std::string nodename = "Node";
    nodename.append(std::to_string(hash_fn(verbose_name)));
    return nodename;
}


void visualize(std::forward_list<std::pair<MessagingNode, MessagingNode>> node_pairs, std::string output_path) {
    MessageMap mmap = buildMessageMap(std::move(node_pairs));

    std::ofstream graph_file(output_path.c_str());
    if (graph_file.good()) {
        // Output file open, let's get started
        std::cout << "[INFO] Started writing the message graph...  ";

        graph_file << "digraph \"Generated Message Graph\" {" << std::endl \
        << "\tlabel=\"Generated Message Graph\";" << std::endl << std::endl;

        for (std::pair<std::string, std::forward_list<std::pair<std::string, std::string>>> item: mmap) {
            std::string nodename = getNodeName(item.first);
            // TODO: might need to insert manual linebreaks...
            graph_file << "\t" << nodename << " [shape=box,label=\"" << item.first << "\"]" << std::endl;

            for (std::pair<std::string, std::string> connection: item.second) {
                graph_file << "\t" << nodename << " -> " << getNodeName(connection.first) \
                << " [label = \"" << connection.second << "\"];" << std::endl;
            }
        }

        graph_file << "}" << std::endl;

        graph_file.close();

        std::cout << "Done!" << std::endl;
    }
    else
        std::cerr << "[ERROR] Could not open output file!" << std::endl;
}
