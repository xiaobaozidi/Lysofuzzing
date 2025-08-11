#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include "GraphPass.h"

using namespace llvm;
using namespace std;

class CallGraph {

private:

  GlobalContext *Ctx;


public:
  
  CallGraph(GlobalContext *Ctx_) : Ctx(Ctx_) {}  

  void addEdge(Function *srcNode, FuncSet &destNode);
  void addNode(Function *Node, IdentifierType ID);
  void addValToFuncMap(Function *func);
  void addCallerMap(Function *callee, CallBase* CB);

  //Auxiliary functions 
  void printNode();
  void printCallGraph();
    
};

#endif

