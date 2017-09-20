#include "visualizer.hpp"

using namespace llvm;

std::hash<std::string> hash_fn;
std::hash<Instruction*> hash_inst;

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


NodeMap buildNodeMap(const std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* node_pairs) {
    NodeMap nmap = NodeMap();

    for (std::pair<MessagingNode*, MessagingNode*> pair: *node_pairs) {
        nmap[pair.first->nspace][pair.first->instr->getDebugLoc()->getLine()].insert(pair.first);
        nmap[pair.second->nspace][pair.second->instr->getDebugLoc()->getLine()].insert(pair.second);
    }

    return nmap;
}


/**
 Generate the internal name of a node in the graph by hashing its description.

 @param verbose_name The verbose node name.
 @return The hashed node name following the scheme "Node<hash>".
 */
std::string getNodeName (std::string verbose_name) {
    std::string nodename = "Node";
    nodename.append(std::to_string(hash_fn(verbose_name)));
    return nodename;
}


void visualize(const std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* node_pairs, std::string output_path) {
    // generate Message and node maps that contain information about the nodes and the messages exchanged
    MessageMap mmap = buildMessageMap(node_pairs);
    NodeMap nmap = buildNodeMap(node_pairs);

    std::ofstream graph_file(output_path.c_str());
    if (graph_file.good()) {
        // Output file open, let's get started
        outs() << "[INFO] Started writing the message graph...  ";

        // write the graph header
        graph_file << "digraph structs {" << std::endl \
        << "\tlabel=\"Generated Message Graph\";" << std::endl \
        << "\trankdir=LR;" << std::endl \
        << "node [shape=record];" << std::endl << std::endl;

        // first, print the node definitions
        for (std::pair<std::string, std::map<long, std::unordered_set<MessagingNode*>>> item: nmap) {
            std::string nodename = getNodeName(item.first);

            // graph_file << "\t" << nodename << " [shape=box,label=\"" << item.first << "\"]" << std::endl;

            // first emit node name, then send/recv nodes
            graph_file << "\t" << nodename \
                       << " [label=\"" << item.first;
            for (std::pair<long, std::unordered_set<MessagingNode*>> node: item.second)
                for (MessagingNode* instruction: node.second)
                    graph_file << "|<" << std::to_string(hash_inst(instruction->instr)) \
                               << "> Line: " << node.first; // TODO: More info here?
            graph_file << "\"]" << std::endl;
        }

        for (std::pair<std::string, std::forward_list<std::pair<MessagingNode*, MessagingNode*>>> item: mmap) {
            std::string nodename = getNodeName(item.first);

            for (std::pair<MessagingNode*, MessagingNode*> connection: item.second) {
                graph_file << "\t" << nodename << ":" << std::to_string(hash_inst(connection.first->instr)) \
                    << " -> " << getNodeName(connection.second->nspace) << ":" << std::to_string(hash_inst(connection.second->instr)) \
                    << " [label = \"" << connection.first->type;

                // add info about sent data (if available)
                if (connection.first->assignment != -1)
                    graph_file << ": " << connection.first->assignment;

                // graph_file << "\\n Receive at: " << connection.second->instr->getDebugLoc()->getLine();
                // if (connection.second->usage.first != Unchecked && connection.second->usage.first != DirectUse)
                //     graph_file << "\\n Handled at Line " << connection.second->usage.second->getDebugLoc()->getLine();
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
