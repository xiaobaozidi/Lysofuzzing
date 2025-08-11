/*
 * CFGReachabilityAnalysis.h
 *
 * Basic CFG reachabilityAnalysis
 *
 *
 */
#ifndef REACHABILITY_ANALYSIS_H
#define REACHABILITY_ANALYSIS_H


#include "GraphPass.h"

using namespace llvm;

class CFGReachability {
   
public:

  using ReachableSet = llvm::BitVector;

  CFGReachability(const Function &F);
  
  ~CFGReachability();

  //Returns true if the block 'Dst' can be reached from block 'Src'.
  bool isReachable(const BasicBlock *Src, const BasicBlock *Dst);

  
private:


  ReachableSet analyzed;
  
  ReachableSet* reachable;
  
  //ID mapping
  std::vector<const BasicBlock *> IDtoBB;
  
  std::unordered_map<const BasicBlock *, IdentifierType> BBID;

  void mapReachability(const BasicBlock *Dst);

};

class CFGraph {

public:

  CFGraph(GlobalContext **Ctx_, const Function &F);
  
  ~CFGraph();

  void buildAdjMatrix(const Function &F);

  std::set<IdentifierType> getReachableBB(IdentifierType id, const Function* func);

  void DFSUtil(unsigned id, std::set<IdentifierType>& visited);
  
  void printGraph();
  
  void printAdjMatrix();

private:

  GlobalContext **Ctx;

  llvm::BitVector* adjMatrix;
  
  std::unordered_map<IdentifierType, const BasicBlock *> ID2BB;
  
  std::unordered_map<const BasicBlock *, IdentifierType> BB2ID;

  std::unordered_map<const BasicBlock *, IdentifierType> BB2RealID;

  std::unordered_map<IdentifierType, const BasicBlock *> RealID2BB;

  IdentifierType funSize;

  IdentifierType numBBHasID;
  
  string funName;

};




class CFGReachabilityAnalysis {

  private:

    GlobalContext *Ctx;

    unsigned int vertixSize;

    std::unordered_map<const Function *, CFGReachability *> funcReachMap;

    std::unordered_map<const Function *, CFGraph *> funCFGMap;

    std::set<IdentifierType> AllreachableBBs;
  
    CFGReachability* getReachability(const Function*);
  
  public:


    CFGReachabilityAnalysis(GlobalContext *Ctx_) : Ctx(Ctx_) {};
        

    ~CFGReachabilityAnalysis();

    
   // Returns true if the block 'Dst' can be reached from block 'Src'.
   bool isBBReachable(const BasicBlock *Src, const BasicBlock *Dst);

   /// Determine reachability within one basic block.
   bool isReachableInBlock(const Instruction *Src, const Instruction *Dst) const;

   // Check the reachability from any two instructions in the same function
   bool isReachable(const Instruction *Src, const Instruction *Dst);

   // Initializes the adjacent matrix for callgraph
   void initAdjMatrix();

   // Build the adjacent matrix for callgraph
   void buildAdjMatrix(); 
  
   // Build the function to CFG map
   void buildFunCFGMap(const Function* func);

   // Get reachable BB inside the function
   void getFunIntraBB(const Function* func, IdentifierType id);

   // Print reachable BB inside the function
   void printIntraBB();
   /* At the beginning, vertixSize is 0. We update it Once we iterate over
    * function loop */  
   void setVertixSize(unsigned int size);

   void PrintFunCFG(const Function* func);
};

#endif
