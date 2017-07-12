#ifndef visualizer_hpp
#define visualizer_hpp

#include <forward_list>
#include <iostream>
#include <fstream>

#include "llvm/IR/Module.h"

#include "types.hpp"

// Function definitions
void visualize(std::forward_list<std::pair<MessagingNode, MessagingNode>> node_pairs, std::string output_path);

#endif /* visualizer_hpp */
