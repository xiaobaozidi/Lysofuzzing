#ifndef GRAPH_PASS_H
#define GRAPH_PASS_H

#include "llvm/Pass.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CFG.h"

//standard C++ headers
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <list>
#include <unordered_set>
#include <set>
#include <map>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <typeinfo>
#include <boost/algorithm/string.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


using namespace llvm;
using namespace std;
using namespace boost::algorithm;


typedef std::uint64_t IdentifierType;
typedef llvm::SmallPtrSet<llvm::Function*, 8> FuncSet;
typedef llvm::SmallPtrSet<llvm::CallBase*, 8> CallBaseSet;
typedef DenseMap<llvm::Function*, FuncSet> CalleeMap;
typedef DenseMap<llvm::Function*, FuncSet> IndirectCalleeMap;
typedef DenseMap<llvm::Function*, CallBaseSet> IndirectCallSiteMap;
typedef unordered_map<llvm::Function*, IdentifierType> FunIdentifiersMap;
typedef unordered_map<llvm::Value*, llvm::Function*> ValueToFuncMap;
typedef DenseMap<Function*, CallBaseSet> CallerMap;
typedef unordered_map<IdentifierType, std::string> BBIdToNameMap;


struct GlobalContext {

  //Map a function to all direct callee functions.
  //eg: Caller1->Callee1
  //    Caller1->Callee2
  //    Caller1->callee3
  //    Caller1->callee4
  CalleeMap CalleeMaps;

  //Map a function(callee) to all callsite instructions.
  CallerMap CallerMaps;

  //Map a function to all potential in-direct callee functions by Type Analysis.
  IndirectCalleeMap TypeIndirectCallMap;

  //Map a function0. to all potential in-direct callee functions by Type Analysis.
  IndirectCallSiteMap IndirectCallSite;

  //Map a function to all potential in-direct callee functions by Point-to Analysis.
  IndirectCalleeMap AliasIndirectCallMap;

  //Map a function to unique ID representation.
  FunIdentifiersMap FunIdMap;

  //Map a value to function.
  ValueToFuncMap ValToFuncMap;

  //The functions that have their address assigned to function pointers are
  //known as address-taken functions.
  FuncSet AddressTakenFuncs;

  //Create Adjacent Matrix for function-level reachability analysis.
  llvm::BitVector* adjMatrix;

  //Map a unique basic block ID to unique basic block name
  BBIdToNameMap BBIdName;

  string DotFile;
};


#endif
