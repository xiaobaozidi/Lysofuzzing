/*
 * @Author Andrew Bao
 * @email: bao00065@umn.edu
 *
 */

#include "CallGraph.h"

using namespace llvm;


void CallGraph::addEdge(Function *srcNode, FuncSet &destNode){
	
      for (auto callee: destNode){
          Ctx->CalleeMaps[srcNode].insert(callee);

      }	  
}


void CallGraph::addCallerMap(Function *callee, CallBase* CB){
      Ctx->CallerMaps[callee].insert(CB);

}


void CallGraph::addNode(Function *Node, IdentifierType ID){
     Ctx->FunIdMap.insert(std::make_pair(Node, ID));
}


void CallGraph::addValToFuncMap(Function *func) { Ctx->ValToFuncMap[func] = func; }


void CallGraph::printNode(){
    for (auto pairs: Ctx->FunIdMap){
        errs() << "NODE is:"<< pairs.first->getName().str() << ": " <<pairs.second << "\n";
    }   
}

std::string getFunFileName(Function &F) {
    // Iterate over all basic blocks in the function
    for (const auto &BB : F) {
        // Iterate over all instructions in the basic block
        for (const auto &I : BB) {
            if (DILocation *Loc = I.getDebugLoc()) { // Get debug location, if available
               std::string filename = Loc->getFilename().str();
                if (!filename.empty()) {
                    return filename; // Return the filename if it's not empty
                }
            }
        }
    }

    return ""; // Return an empty string if no debug location with a filename is found
}



void CallGraph::printCallGraph(){
  
  std::string dotfiles( Ctx->DotFile + "/dot-files");
  if (!llvm::sys::fs::exists(dotfiles)){
    llvm::sys::fs::create_directory(dotfiles);
  }

  std::string GraphName( dotfiles + "/callgraph.dot");
  std::error_code error;
  raw_fd_ostream O(GraphName, error);

  O << "digraph \"Call Graph\"{\n";
  O << "label=\"Call Graph\";\n";

  for (auto pairs : Ctx->FunIdMap){

    Function *F = pairs.first;
    unsigned id = pairs.second;
    //errs() << F->getName().str() << ":" << id<< "\n";    
     O << id << " [shape=record, filename=\"{" << getFunFileName(*F) << "}\"" << "label=\"{" << F->getName().str()
      << "}\", indirect callsite=\"{";

    if (!Ctx->IndirectCallSite[F].empty()){
       for (auto callInst: Ctx->IndirectCallSite[F]){
           DILocation *Loc = callInst->getDebugLoc();
           string filename = Loc->getFilename().str();
           unsigned line = Loc->getLine();
           O << " " << filename << ":" << line << " "; 
       }
    } 
    O << "}\"];\n";
    
    //Output direct call
    if (!Ctx->CalleeMaps[F].empty()){
       for (auto callee: Ctx->CalleeMaps[F]){
	   if (Ctx->FunIdMap.find(callee) != Ctx->FunIdMap.end()) {   
              IdentifierType calleeID = Ctx->FunIdMap[callee];
              O << id << " -> " << calleeID <<";\n";
	   }   
       }   
    }
     /*Indirect call case 1:
     *
     * When alias analysis is empty but type analysis is not empty, we directly 
     * use the result from type analysis. 
     *
     */

    
    if (Ctx->AliasIndirectCallMap[F].empty() && !Ctx->TypeIndirectCallMap[F].empty()){
       for (auto TypeCallee: Ctx->TypeIndirectCallMap[F]){ 
            if (Ctx->FunIdMap.find(TypeCallee) != Ctx->FunIdMap.end()){
               IdentifierType calleeID = Ctx->FunIdMap[TypeCallee];
               O << id << " -> " << calleeID <<";\n";
	    }
      }
    }
    /*Indirect call case 2:
     *
     *When alias analysis and type analysis are not empty, usually, the result from alias
     *analysis is the subset of type analysis.
     *
     */
    else if (!Ctx->AliasIndirectCallMap[F].empty() && ! Ctx->TypeIndirectCallMap[F].empty()){
       for (auto TypeCallee: Ctx->TypeIndirectCallMap[F]){ 
          for (auto AliasCallee: Ctx->AliasIndirectCallMap[F]){
               if (AliasCallee == TypeCallee){
		  if (Ctx->FunIdMap.find(AliasCallee) != Ctx->FunIdMap.end()){    
                     IdentifierType calleeID = Ctx->FunIdMap[AliasCallee];
                     O << id << " -> " << calleeID <<";\n";
		  }    
               }
          }
       }   
    } 

  }
  O << "}\n";
  O.close();
}



