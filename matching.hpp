#ifndef analyzer_hpp
#define analyzer_hpp

#include <forward_list>
#include <iostream>

#include "types.hpp"

// function definitions
std::forward_list<std::pair<MessagingNode*, MessagingNode*>> analyzeNodes(std::forward_list<MessagingNode>& sends, std::forward_list<MessagingNode>& recvs, bool suppress_parentheses);
std::forward_list<std::pair<MessagingNode*, MessagingNode*>> analyzeNodes(std::pair<std::forward_list<MessagingNode>, std::forward_list<MessagingNode>>& data, bool suppress_parentheses);

#endif /* analyzer_hpp */
