#ifndef POINTER_ANALYSIS_H
#define POINTER_ANALYSIS_H


#include "GraphPass.h"
using namespace std;
using namespace llvm;

class PointerAnalysis {


private: 
 
  GlobalContext *Ctx;

  list<llvm::Instruction *> workList;
  map<llvm::Value *, set<llvm::Value *>> pointToSet;
  Function *resolvePointsTo(Value *val, llvm::SmallPtrSet<llvm::Function*, 8> &functionSet, unordered_map<llvm::Value*, llvm::Function*> ValToFuncMap);
  llvm::SmallPtrSet<llvm::Function*, 8> getPointsToFunctions(Value *val, unordered_map<llvm::Value*, llvm::Function*> ValToFuncMap);

  void addAliasIndirectCallEdge(Function *srcNode, FuncSet &destNode);


public:

  PointerAnalysis(GlobalContext *Ctx_) : Ctx(Ctx_) {}

  void pointsTo(llvm::Value *from, llvm::Value *to);
  void addToWorkList(llvm::Instruction *inst);
  void processWorkList();
  void printPointToSet();
};

#endif

