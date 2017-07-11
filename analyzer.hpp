#ifndef analyzer_hpp
#define analyzer_hpp

#include <forward_list>

#include "types.hpp"

// function definitions
std::forward_list<std::pair<MessagingNode*, MessagingNode*>> analyzeNodes(std::forward_list<MessagingNode>* sends, std::forward_list<MessagingNode>* recvs);
std::forward_list<std::pair<MessagingNode*, MessagingNode*>> analyzeNodes(std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>>* data);

#endif /* analyzer_hpp */
