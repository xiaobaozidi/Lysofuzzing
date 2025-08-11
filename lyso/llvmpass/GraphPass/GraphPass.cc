#include "GraphPass.h"
#include "CallGraph.h"
#include "TypeAnalysis.h"
#include "PointerAnalysis.h"
#include "CFGReachabilityAnalysis.h"

//Define global context here
GlobalContext GlobalCtx;


cl::opt<std::string> TargetsFile(
    "targets",
    cl::desc("Input file containing the target lines of code."),
    cl::value_desc("targets"));

cl::opt<std::string> OutDirectory(
    "outdir",
    cl::desc("Output directory where BBnames.txt, BBcalls.txt, Fnames.txt, Funtargets.txt BBtargets.txt and dot-files are generated."),
    cl::value_desc("outdir"));


namespace llvm {

class PassRegistry;
void initializeGraphPassPass(PassRegistry&);
}

namespace {

  struct GraphPass : public ModulePass {

    static char ID;
    GraphPass() : ModulePass(ID) {}

    bool runOnModule(Module &M) override;


  };

}

char GraphPass::ID = 0;




static void getDebugLoc(const Instruction *I, std::string &Filename,
                        unsigned &Line) {
#ifdef LLVM_OLD_DEBUG_API
  DebugLoc Loc = I->getDebugLoc();
  if (!Loc.isUnknown()) {
    DILocation cDILoc(Loc.getAsMDNode(M.getContext()));
    DILocation oDILoc = cDILoc.getOrigLocation();

    Line = oDILoc.getLineNumber();
    Filename = oDILoc.getFilename().str();

    if (filename.empty()) {
      Line = cDILoc.getLineNumber();
      Filename = cDILoc.getFilename().str();
    }
  }
#else
  if (DILocation *Loc = I->getDebugLoc()) {
    Line = Loc->getLine();
    Filename = Loc->getFilename().str();

    if (Filename.empty()) {
      DILocation *oDILoc = Loc->getInlinedAt();
      if (oDILoc) {
        Line = oDILoc->getLine();
        Filename = oDILoc->getFilename().str();
      }
    }
  }
#endif /* LLVM_OLD_DEBUG_API */
}



bool isBlacklisted(const Function *F) {
  static const SmallVector<std::string, 8> Blacklist = {
    "asan.",
    "llvm.",
    "sancov.",
    "__ubsan_handle_",
    "__instr_fun"
    "__instr_bb"
  };

  for (auto const &BlacklistFunc : Blacklist) {
    if (F->getName().startswith(BlacklistFunc)) {
      return true;
    }
  }

  return false;
}


bool isSourceFileInUsr(const llvm::Function &F) {

    static const std::string Xlibs = "/usr/";

    // Check if the function has at least one basic block
    if (!F.empty()) {
        // Iterate over all basic blocks in the function
        for (const auto &BB : F) {
            std::string filename;
            unsigned line;

            // Iterate over all instructions in the basic block
            for (const auto &I : BB) {
                getDebugLoc(&I, filename, line);
                if (filename.empty() || line == 0) continue;

                // Check if the filename starts with "/usr/"
                if (!filename.compare(0, Xlibs.size(), Xlibs)) return true;
                else return false;
            }
        }
    }
    return false;
}


bool GraphPass::runOnModule(Module &M){

  if (TargetsFile.empty()) {

     perror("Provide target file '-target <file>'");
     return false;
  }

  if (OutDirectory.empty()){

     perror("Provide output directory '-outdir <directory>'");
     return false;

  }


  //We use Targets to store the original target file
  std::vector<std::string> Target;
  //We use Translate target to store the translation information
  std::unordered_set<std::string> TranslateTarget;


  std::unordered_set<unsigned> FTarget;
  std::unordered_set<unsigned> BTarget;

  std::ifstream targetsfile(TargetsFile);
  std::string line;

  /*AB:  Read the targets to FunTargets*/
  while (std::getline(targetsfile, line)){
        if (line.find('-') == std::string::npos){
           Target.push_back(line);
        }else{
           Target.push_back("------------");
        }
  }
  targetsfile.close();

  LLVMContext &C = M.getContext();
  IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
  IntegerType *Int16Ty  = IntegerType::getInt16Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);


  /*AB: Initialize different analysis */
  CallGraph* G = new CallGraph(&GlobalCtx);
  PointerAnalysis* P = new PointerAnalysis(&GlobalCtx);
  TypeAnalysis* T = new TypeAnalysis(&GlobalCtx);
  CFGReachabilityAnalysis* CFGR = new CFGReachabilityAnalysis(&GlobalCtx);
  T->DL = &(M.getDataLayout());
  T->Int8PtrTy = Type::getInt8PtrTy(M.getContext());
  T->IntPtrTy = T->DL->getIntPtrType(M.getContext());


  GlobalCtx.DotFile = OutDirectory;

  /*AB: Create the files*/
  std::ofstream bbnames(OutDirectory + "/BBnames.txt", std::ofstream::out | std::ofstream::app);
  std::ofstream bbcalls(OutDirectory + "/BBcalls.txt", std::ofstream::out | std::ofstream::app);
  std::ofstream fnames(OutDirectory + "/Fnames.txt", std::ofstream::out | std::ofstream::app);
  std::ofstream ftargets(OutDirectory + "/Ftargets.txt", std::ofstream::out | std::ofstream::app);
  std::ofstream btargets(OutDirectory + "/Btargets.txt", std::ofstream::out | std::ofstream::app);
  std::ofstream fun_targets(OutDirectory + "/Funtargets.txt", std::ofstream::out | std::ofstream::app);
  std::ofstream bb_targets(OutDirectory + "/BBtargets.txt", std::ofstream::out | std::ofstream::app);

  /*AB Address-taken function collection */
  for (auto &F : M){
      if ( F.empty() ||F.isDeclaration() || F.isIntrinsic() || isBlacklisted(&F) || isSourceFileInUsr(F))
              continue;
      // Address taken function collection. It is used for type analysis
      if (F.hasAddressTaken())
          T->addFunc(&F);
  
    
  }

  unsigned fun_id = 0;
  unsigned bb_id = 0;

  for (auto &F : M){

       std::string pathname;

       if ( F.empty() ||F.isDeclaration() || F.isIntrinsic() || isBlacklisted(&F) || isSourceFileInUsr(F))
              continue;

       if (MDNode *NF = F.getMetadata("lyso_fun_id") ) {

            fun_id = cast<ConstantInt>(cast<ValueAsMetadata>(NF->getOperand(0))->getValue())->getZExtValue();

            //errs() << "F is : " << F.getName() << ":" << fun_id << "\n";
            // ID generation
            G->addNode(&F, fun_id);
            G->addValToFuncMap(&F);

            //Build CFG and Reachability map
            CFGR->buildFunCFGMap(&F);
            //CFGR->getReachability(&F);
            for (auto &BB : F){
         
               bool has_bb_id = false;

	       //get bb id from bc file
               auto *TI = BB.getTerminator();
               if (MDNode *NBB = TI->getMetadata("lyso_bb_id")) {
                  bb_id = cast<ConstantInt>(cast<ValueAsMetadata>(NBB->getOperand(0))->getValue())->getZExtValue();
	          has_bb_id = true;
               }

	       if (has_bb_id == false) continue;
               std::string bb_name("");
               std::string filename;
               unsigned line;

       
               for (auto &I : BB){
                
                   getDebugLoc(&I, filename, line);

                   //if (filename.empty() || line == 0)
                      //continue;
                   pathname = filename;

                   std::size_t found = filename.find_last_of("/\\");
                   if (found != std::string::npos)
                      filename = filename.substr(found + 1);
                

                   // changed by zyt to testing 20221018
                   // bb_name = filename + ":" + std::to_string(line);
                   bb_name = std::to_string(bb_id);

                   //AB: Check if the basic block is the target basic block
                   for (auto &target : Target) {

                       std::size_t found = target.find_last_of("/\\");

                       if (found != std::string::npos){
                          target = target.substr(found + 1);
                       }

                       std::size_t pos = target.find_last_of(":");
                       std::string target_file = target.substr(0, pos);
                       unsigned target_line = atoi(target.substr(pos + 1).c_str());

                       if (!target_file.compare(filename) && target_line == line){

                          std::stringstream target_info;
                          target_info << target << ", "
                                  << fun_id << ", " << bb_id << "\n";
                          //errs() << target_info.str() << "\n";
                          TranslateTarget.insert(target_info.str());
                          FTarget.insert(fun_id);
                          BTarget.insert(bb_id);
                          CFGR->getFunIntraBB(&F, bb_id);
                       }
                   }


                   if (isa<StoreInst>(I)){

                      StoreInst *SI = dyn_cast<StoreInst>(&I);

                      Value *to = SI->getValueOperand()->stripPointerCasts();

                      Value *from = SI->getPointerOperand()->stripPointerCasts();


                     //Add Points-To
                     P->pointsTo(from, to);
                     //Add to Worklist
                     P->addToWorkList(&I);
                   }

                   else if (isa<LoadInst>(I)){

                       LoadInst *LI = dyn_cast<LoadInst>(&I);

                       Value *to = LI->getPointerOperand()->stripPointerCasts();
                       Value *from = dyn_cast<Value>(LI);

                       // Add Points-To
                       P->pointsTo(from, to);
                       // Add to Worklist
                       P->addToWorkList(&I);
                   }
		   else if (CallBase *CB = dyn_cast<CallBase>(&I)){
                       FuncSet FS;

		       Value *CV = CB->getCalledOperand();
 
                       //Indirect Call
                       if (CB->isIndirectCall()){
                       
 
                          P->addToWorkList(&I);
                          T->findCalleesByType(CB, FS);
                          /* The indirect call is for type analysis */
                          T->addTypeIndirectCallEdge(&F, FS);
                          /* This is used for debug purpose */
                          T->addIndirectCallSite(&F, CB);

		          for(auto callee: FS){ 
		       	     bbcalls << bb_name << "," << callee->getName().str() << "\n";
                          }

                       } else {

		          Function *CF = nullptr;
                          // Direct Call
                          // If the called value is a bitcast, resolve it to the actual function
                          CF = llvm::dyn_cast<llvm::Function>(CB->getCalledOperand()->stripPointerCasts());

		          if ( CF && !CF->isIntrinsic() && !CF->isDeclaration() && !isBlacklisted(CF)){

		             //errs() << "Called Function is:" << CF->getName().str() << "\n";		  
                             FS.insert(CF);
                             /* This is used to build call graph edge */
                             G->addEdge(&F, FS);
                             /* This is used to collect the basic block that used for InterProccedure CFG */
                             G->addCallerMap(CF, CB);

                             bbcalls << bb_name << "," << CF->getName().str() << "\n";
			  }   
                       }       
		  }

             }
             if (!bb_name.empty()) {
                BB.setName(bb_name + ":");
          
             }else{
                BB.setName(std::to_string(bb_id) + ":");
             }
/*
             if (!BB.hasName()) {
                std::string newname = bb_name + ":";
                Twine t(newname);
                SmallString<256> NameData;
                StringRef NameRef = t.toStringRef(NameData);
                MallocAllocator Allocator;
                BB.setValueName(ValueName::Create(NameRef, Allocator));
             }

*/

             bbnames << BB.getName().str() << "\n";
             /* Collect basic block unique id and its corrsponding name  */
             GlobalCtx.BBIdName.insert(std::make_pair(bb_id, bb_name));



       }
       fnames << pathname << ":" << F.getName().str() << "," << fun_id<<"\n";
       
       //Print out CFG
       CFGR->PrintFunCFG(&F);
      }
  }
  //We know in translate target set, there could be some duplications due to
  //macro, bear with it and we only need one 

  std::map<string, uint> target_fun_pair;
  std::map<string, uint> target_bb_pair;
  

  for (auto iter: TranslateTarget){
         
         std::vector<std::string> splits;
         split(splits, iter, is_any_of(","));
         std::string file_line = splits[0];
         unsigned int fun_id = stoi(splits[1]);
         unsigned int bb_id = stoi(splits[2]);
         
         target_fun_pair.insert(std::make_pair(file_line, fun_id));
         target_bb_pair.insert(std::make_pair(file_line, bb_id));

  } 
    
  for (auto iter: Target){
  
     if (iter[0] != '-' ){
        fun_targets << target_fun_pair[iter] << "\n";
        bb_targets << target_bb_pair[iter] << "\n";
    }else{
        fun_targets << "----------------\n";
        bb_targets << "---------------\n";

    }
  }


  for (auto iter: FTarget){
      ftargets << std::to_string(iter) << "\n";
  }

  for (auto iter: BTarget){
      btargets << std::to_string(iter) << "\n";
  }

  /* Conduct Point-to Analysis for indirect-call */
  //P->printPointToSet();
  //P->processWorkList();




  /* Conduct Reachability Analysis  */
  CFGR->setVertixSize(GlobalCtx.FunIdMap.size());
  //errs() << "Size is" << GlobalCtx.FunIdMap.size() << "\n";


  //G->printNode();
  /* Construct the final call graph result with in-direct call support*/
  G->printCallGraph();
  
  CFGR->printIntraBB();


  /* Print adddress-taken functions */
  T->printAddressTakenFunc();

  /* Destory the analysis  */
  delete G;
  delete P;
  delete T;
  delete CFGR;

  /*Close file descriptor */
  bbnames.close();
  bbcalls.close();
  fnames.close();
  ftargets.close();
  btargets.close();
  fun_targets.close();
  bb_targets.close();
  

  return true;
}


INITIALIZE_PASS_BEGIN(GraphPass, "GraphPass", "Print the function name", false, false)
INITIALIZE_PASS_END(GraphPass, "GraphPass", "Print the function name", false, false)

static RegisterPass<GraphPass> X("GraphPass", "Print the function name",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerGraphPass(const PassManagerBuilder &,
    legacy::PassManagerBase &PM) {
  PM.add(new GraphPass());
}
static RegisterStandardPasses
RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
    registerGraphPass);
































