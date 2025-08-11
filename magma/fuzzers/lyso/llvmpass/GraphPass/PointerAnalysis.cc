#include "PointerAnalysis.h"

void PointerAnalysis::addToWorkList(Instruction *inst){
  workList.push_back(inst);
}

void PointerAnalysis::pointsTo(llvm::Value *from, llvm::Value *to){
  pointToSet[from].insert(to);
}


void PointerAnalysis::printPointToSet() {

  errs() << "PointToSet: \n";
  for (auto const &set : pointToSet) {
    errs() << *set.first << "------------------>";
    for (auto const &addr : set.second) {
      errs() << *addr << ":::::::::::::::::";
    }   
    errs() << "\n\n\n\n\n";

  }
  errs() << "End \n";

}



Function *
PointerAnalysis::resolvePointsTo(Value *val, llvm::SmallPtrSet<llvm::Function*, 8> &functionSet, 
    unordered_map<llvm::Value*, llvm::Function*> ValToFuncMap) {

  // Collect the Function.
  if (ValToFuncMap[val] != nullptr) {
    //errs() << "ValTOFuncMap: " << ValToFuncMap[val] << "\n"; 
    return ValToFuncMap[val];
  }

  
  set<Value *> values = pointToSet[val];
  //for (auto val: values)
  //    errs() << "Point-to value is:" << *val << "\n";


  for (Value *v : values) {
    Function *func = resolvePointsTo(v,  functionSet, ValToFuncMap);
    if (func != nullptr) {
      //errs()<< "func is " << *func << "\n";
      functionSet.insert(func);
    }   
  }
  return nullptr;
}


llvm::SmallPtrSet<llvm::Function*, 8> PointerAnalysis::getPointsToFunctions(Value *val, 
    unordered_map<llvm::Value*, llvm::Function*> ValToFuncMap) {
  
  llvm::SmallPtrSet<llvm::Function*, 8> FS;
  //errs()<< "value is:" << *val <<  "\n";
  resolvePointsTo(val, FS, ValToFuncMap);
  return FS;
} 


//This is used for point to analysis
void PointerAnalysis::addAliasIndirectCallEdge(Function *srcNode, FuncSet &destNode){
    for (auto callee: destNode)
        Ctx->AliasIndirectCallMap[srcNode].insert(callee);
}


void PointerAnalysis::processWorkList() {
  while (!workList.empty()) {
    llvm::Instruction *inst = workList.front();

    if (isa<CallInst>(inst)) {
      //errs() << "call inst is:" << *inst  << "\n";

      CallInst *callInst = dyn_cast<CallInst>(inst);

      
      llvm::SmallPtrSet<llvm::Function*, 8> FS =  getPointsToFunctions(
          callInst->getCalledOperand()->stripPointerCasts(),
          Ctx->ValToFuncMap);

      //Add to in-direct call edge
      addAliasIndirectCallEdge(callInst->getFunction(), FS);


      //for (auto callee: FS){
      //    errs() << "Indirect call Edge is(Alias Analysis):  " << callInst->getFunction()->getName().str() << " <=> " << callee->getName().str() << "\n";
      //}

    }

    workList.pop_front();
  }
}
