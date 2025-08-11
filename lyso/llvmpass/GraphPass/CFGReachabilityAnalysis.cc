#include "CFGReachabilityAnalysis.h"


//Constructor
CFGReachability::CFGReachability(const Function& F) :
           analyzed(F.size() + 1, false) {

    reachable = new ReachableSet[F.size() + 1];
    unsigned int i = 1;

    for (auto &BB: F) {
        IDtoBB.push_back(&BB);
        BBID[&BB] = i;
        errs() << BB << "\n";
        reachable[i].resize(F.size() + 1, false);
        i++;
    }

}

CFGReachability::~CFGReachability() {
    delete [] reachable;
}


//Maps reachability to a common node by walking the predecessors of the
//destination node.
void CFGReachability::mapReachability(const BasicBlock *Dst) {
     llvm::BitVector visited(analyzed.size());

     ReachableSet &DstReachability = reachable[BBID[Dst]];

     //DFS: Search all the reachable nodes from Dst
     
     std::vector<const BasicBlock *>worklist;
     worklist.push_back(Dst);
     bool firstRun = true;

     while (!worklist.empty()) {
      
           const BasicBlock *block = worklist.back();
           worklist.pop_back();
           unsigned blockID = BBID[block];
           if (visited[blockID])
              continue;
           visited[blockID] = true;

           // Update reachability information for this node 
           if (!firstRun) {
              //Don't insert Dst -> Dst unless it was a predecessor of itself
              DstReachability[blockID] = true;
           }else
            firstRun = false;
     
          //Add the predecessors to the worklist
          for (auto I = pred_begin(block), E = pred_end(block); I != E; ++I){
              const BasicBlock* pred_bb = *I;
              worklist.push_back(pred_bb);
          
          }     
     }
}

void CFGraph::buildAdjMatrix(const Function& F){
  for (auto &BB: F) {
      
      const BasicBlock *curBB = &BB;
      
      for (const BasicBlock *SuccBB : successors(curBB)){
          
           adjMatrix[BB2ID.at(curBB)][BB2ID.at(SuccBB)] = true; 
             
      }
    
  }
     
           
}


void CFGraph::printAdjMatrix(){
     
     for(unsigned i=1; i< numBBHasID + 1; i++){
        for (unsigned j=1; j< numBBHasID + 1; j++){
            if (adjMatrix[i][j] == true){
               errs() << i << " reach " << j << "\n";

            }
        
        }
     }
}


//Constructor initializes the adjacent matrix for a particular function
CFGraph::CFGraph(GlobalContext** Ctx_, const Function& F): 
        Ctx(Ctx_), funSize(F.size()), funName(F.getName().str()){
    
    numBBHasID = 0;		
    for (auto &BB: F){
    
        auto *TI = BB.getTerminator();
        if (MDNode *NBB = TI->getMetadata("lyso_bb_id")) {
           numBBHasID++;
	   IdentifierType bb_id = cast<ConstantInt>(cast<ValueAsMetadata>(NBB->getOperand(0))->getValue())->getZExtValue();
	   //errs() << "bb_id is " << bb_id << "\n";
	   BB2RealID[&BB] = bb_id;
           RealID2BB[bb_id] = &BB;
        }
    }

    //errs() << "numBBHasID is "<< numBBHasID << "\n";
    adjMatrix = new BitVector[numBBHasID + 1];

    IdentifierType id = 1;
    for (auto &BB: F) {
        if (BB2RealID.find(&BB) != BB2RealID.end()) {
	    ID2BB[id] = &BB;
            BB2ID[&BB] = id;
            adjMatrix[id].resize(numBBHasID + 1, false);
	    id++;
	}
    }

}
	
CFGraph::~CFGraph() {
    delete [] adjMatrix;
}


void CFGraph::DFSUtil(unsigned id, std::set<IdentifierType>& visited) {
    visited.insert(id);

    for (unsigned i = 1; i <= numBBHasID; ++i) {
        if (adjMatrix[i][id] == true && visited.find(i) == visited.end()) {
            DFSUtil(i, visited);
        }
    }
}

std::set<IdentifierType> CFGraph::getReachableBB(IdentifierType id, const Function* func){

     std::set<IdentifierType> reachableBBIDs;
     std::set<IdentifierType> reachableBBRealIDs;


     const BasicBlock *targetBB = RealID2BB[id];
     unsigned bbIndex = BB2ID[targetBB];
     DFSUtil(bbIndex, reachableBBIDs);
     for (IdentifierType i: reachableBBIDs){
         reachableBBRealIDs.insert(BB2RealID[ID2BB[i]]);
         //errs() << "reachable id is:" << BB2RealID[ID2BB[i]] << "\n"; 
     }
     reachableBBRealIDs.insert(id);    
 
     return reachableBBRealIDs;
}


void CFGraph::printGraph(){

     std::string dotfiles( (*Ctx)->DotFile + "/dot-files");
     
     if (!sys::fs::exists(dotfiles)){
        sys::fs::create_directory(dotfiles);
     }

     std::string Graphname(dotfiles + "/cfg." + funName + ".dot");
     std::error_code error;
     raw_fd_ostream O(Graphname, error);
     O << "digraph \"CFG for '" + funName + "\' function\" {\n";
  
     for(unsigned i=1; i< numBBHasID + 1; i++){
        
         O << "\t" << BB2RealID[ID2BB[i]] << " [shape=record, label= \"{";
         
         O << BB2RealID[ID2BB[i]]; 
         
         O << "}\" ];\n"; 
         for (unsigned j=1; j< numBBHasID + 1; j++){ 
               
             if (adjMatrix[i][j] == true ){
                O << "\t" << BB2RealID[ID2BB[i]] << " -> " << BB2RealID[ID2BB[j]] << ";\n"; 
             }

         }

     }
     O << "}\n";
     O.close();
}




bool CFGReachability::isReachable(const BasicBlock *Src, const BasicBlock* Dst) {
  
     assert(BBID.count(Dst) && BBID.count(Src) && "SrcBB or DstBB are not label");

     const unsigned DstBlockID = BBID.at(Dst);

     if (!analyzed[DstBlockID]){
        mapReachability(Dst);
        analyzed[DstBlockID] = true;
     
     }

     //Return the cached result
     return reachable[DstBlockID][BBID.at(Src)];
}


void CFGReachabilityAnalysis::setVertixSize(unsigned int size){
     vertixSize = size;    
}


void CFGReachabilityAnalysis::initAdjMatrix(){
          Ctx->adjMatrix = new llvm::BitVector[vertixSize + 1];
          for(int i= 1; i<vertixSize; i++){
            Ctx->adjMatrix[i].resize(vertixSize + 1, false);
          }
};


//Destructor 
CFGReachabilityAnalysis::~CFGReachabilityAnalysis() {
  //for (auto pair: funcReachMap) {
  //    delete pair.second;
  //}

  for (auto pair: funCFGMap) {
      delete pair.second;
  }

}


void CFGReachabilityAnalysis::buildFunCFGMap(const Function* func){
      
         auto It = funCFGMap.find(func);
        
         //cfg graph not exists. 
         if (It == funCFGMap.end()) {
           
            CFGraph* G = new CFGraph(&Ctx, *func);
            funCFGMap[func] = G;
            G->buildAdjMatrix(*func);
             
            //G->printGraph();
         }
}


void CFGReachabilityAnalysis::PrintFunCFG(const Function* func){
     auto It = funCFGMap.find(func);
     //cfg graph exists.
     if (It != funCFGMap.end()) {
        It->second->printGraph();    
     }     

}

void CFGReachabilityAnalysis::getFunIntraBB(const Function* func, IdentifierType id){
    

     //errs() << "bb id is:" << id  << "\n";
     auto It = funCFGMap.find(func);
     //cfg graph exists.
     if (It != funCFGMap.end()) {
        std::set<IdentifierType> reachableBBs = It->second->getReachableBB(id, func);
        AllreachableBBs.insert(reachableBBs.begin(), reachableBBs.end());
        //It->second->printAdjMatrix();
     }
}


void CFGReachabilityAnalysis::printIntraBB(){

     std::string Graphname(Ctx->DotFile + "/IntraBB.txt");
     std::error_code error;
     raw_fd_ostream O(Graphname, error);

     // Writing the reachable basic block IDs to the file
     for (IdentifierType bbID : AllreachableBBs) {
        O << bbID << "\n";
     }

     O.close();
}



CFGReachability* CFGReachabilityAnalysis::getReachability(const Function* func){

    CFGReachability* result;
    auto It = funcReachMap.find(func);

    if (It == funcReachMap.end()) {
       
       //We build the reachability results on demand
       result = new CFGReachability(*func);
       funcReachMap[func] = result;
    }
    else {
        
         result = It->second;
    }
    
    return result;
}



bool CFGReachabilityAnalysis::isBBReachable(const BasicBlock *Src, const BasicBlock *Dst) {
     
   assert( Src->getParent() == Dst->getParent()  && 
       "Cannot query two basic blocks in different functions");

   CFGReachability* CFGR_BB = getReachability(Src->getParent());
   return CFGR_BB->isReachable(Src, Dst);

}


//Sometime in the same block, src instruction comes after dest instruction, in this case, it is
//unreachable. 
bool CFGReachabilityAnalysis::isReachableInBlock(const Instruction* Src, 
    const Instruction* Dst) const {
     
   assert(Src->getParent() == Dst->getParent() && 
       "isReachableInBlock is called on two instructions that belongs to different Basic Blocks");
   
   auto *srcBB = Src->getParent();
   for (auto &I : *srcBB) {
       if (&I == Src) {
          return true;
       }else if (&I == Dst){
          return false;
       }
   
   
   }
   return false;
}

bool CFGReachabilityAnalysis::isReachable(const Instruction* Src, const Instruction* Dst) {

     auto *srcBB = Src->getParent();
     auto *dstBB = Dst->getParent();

     if (srcBB != dstBB){
        // If the two instruction are located in different BB
        return isBBReachable(srcBB, dstBB);
     } else{
        // If this two BB are in the same BB.
        return isReachableInBlock(Src, Dst);
     }

}
