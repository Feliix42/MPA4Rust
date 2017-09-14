#ifndef analysisguide_hpp
#define analysisguide_hpp

#include <forward_list>
#include <string>
#include <list>
#include <queue>
#include <unordered_set>
#include <sstream>

#include <iostream>
#include "llvm/Support/raw_ostream.h"

//Debug Information and Metadata
#include "llvm/IR/DebugInfoMetadata.h"

// name demangling
#include "llvm/Demangle/Demangle.h"

#include "types.hpp"
#include "visualizer.hpp"
#include "properties.hpp"

std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* analyzeGuided(const std::forward_list<std::pair<MessagingNode*, MessagingNode*>>* node_pairs);

#endif /* analysisguide_hpp */
