#include "visualizer.hpp"

using namespace llvm;

MessageMap buildMessageMap(const std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* node_pairs) {
    MessageMap mmap = MessageMap();

    for (std::pair<MessagingNode*, MessagingNode*> pair: *node_pairs) {
        mmap[pair.first->nspace].push_front(pair);

        // if a node is never sending anything and just receiving, we risk having no information about it in the graph
        //  -> therefore, for every receiver an empty node is inserted
        if (mmap.find(pair.second->nspace) == mmap.end())
            mmap.insert(std::make_pair(pair.second->nspace, std::forward_list<std::pair<MessagingNode*, MessagingNode*>>::forward_list()));
    }

    return mmap;
}


/**
 Generate the internal name of a node in the graph by hashing its description.

 @param verbose_name The verbose node name.
 @return The hashed node name following the scheme "Node<hash>".
 */
std::string getNodeName (std::string verbose_name) {
    // TODO: maybe make the hashing function global
    std::hash<std::string> hash_fn;
    std::string nodename = "Node";
    nodename.append(std::to_string(hash_fn(verbose_name)));
    return nodename;
}


void visualize(const std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* node_pairs, std::string output_path) {
    MessageMap mmap = buildMessageMap(node_pairs);

    std::ofstream graph_file(output_path.c_str());
    if (graph_file.good()) {
        // Output file open, let's get started
        outs() << "[INFO] Started writing the message graph...  ";

        graph_file << "digraph \"Generated Message Graph\" {" << std::endl \
        << "\tlabel=\"Generated Message Graph\";" << std::endl << std::endl;

        for (std::pair<std::string, std::forward_list<std::pair<MessagingNode*, MessagingNode*>>> item: mmap) {
            std::string nodename = getNodeName(item.first);
            // TODO: might need to insert manual linebreaks...
            graph_file << "\t" << nodename << " [shape=box,label=\"" << item.first << "\"]" << std::endl;

            for (std::pair<MessagingNode*, MessagingNode*> connection: item.second) {
                graph_file << "\t" << nodename << " -> " << getNodeName(connection.second->nspace) \
                << " [label = \"" << connection.first->type;
                if (connection.first->assignment != -1)
                    graph_file << ": " << connection.first->assignment;
                graph_file << "\\n Receive at: " << connection.second->instr->getDebugLoc()->getLine();
                if (connection.second->usage.first != Unchecked && connection.second->usage.first != DirectUse)
                    graph_file << "\\n Handled at Line " << connection.second->usage.second->getDebugLoc()->getLine();
                graph_file << "\"];" << std::endl;
            }
        }

        graph_file << "}" << std::endl;

        graph_file.close();

        outs() << "Done!\n";
    }
    else
        std::cerr << "[ERROR] Could not open output file!" << std::endl;
}
