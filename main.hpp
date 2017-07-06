#ifndef main_hpp
#define main_hpp

#include <forward_list>
#include <iostream>
#include <memory>
#include <sys/stat.h>

// Boost Filesystem interaction
#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>


#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "scanner.hpp"

#endif /* main_hpp */
