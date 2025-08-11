/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.
*/

#define AFL_LLVM_PASS

#include "../config.h"
#include "../debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"

#include <string>
#include <boost/algorithm/string.hpp>
#include <unordered_set>
#include <iostream>
#include <fstream>



using namespace llvm;
using namespace std;
using namespace boost::algorithm;


cl::opt<std::string> Mode(
    "mode",
    cl::desc("Select a mode between cov(coverage tracking) and insert(insert unique number)"),
    cl::value_desc("mode"));

cl::opt<std::string> DistanceFile(
    "distance",
    cl::desc("Distance file containing the distance of each basic block to the provided targets."),
    cl::value_desc("distance"));

cl::opt<std::string> TraceFile(
    "trace",
    cl::desc("Trace file containing the reachable basic block to the provided targets."),
    cl::value_desc("trace"));

cl::opt<std::string> OutDirectory(
    "outdir",
    cl::desc("Output directory where BBnames.txt, BBcalls.txt, Fnames.txt, Funtargets.txt BBtargets.txt and dot-files are generated."),
    cl::value_desc("outdir"));


namespace {

  class AFLCoverage : public ModulePass {

    public:

      static char ID;
      AFLCoverage() : ModulePass(ID) { }

      bool runOnModule(Module &M) override;
      
      void getDebugLoc(const Instruction *I, std::string &Filename,
                        unsigned &Line);

      bool isBlacklisted(const Function *F);

      bool isSourceFileInUsr(const llvm::Function &F);

  };

}


char AFLCoverage::ID = 0;

void AFLCoverage::getDebugLoc(const Instruction *I, std::string &Filename,
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

bool AFLCoverage::isSourceFileInUsr(const llvm::Function &F) {

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


bool AFLCoverage::isBlacklisted(const Function *F) {
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


bool AFLCoverage::runOnModule(Module &M) {

  bool is_insert = false;

  if (Mode.compare("is_insert") == 0) is_insert = true;

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
  IntegerType *Int16Ty  = IntegerType::getInt16Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {
 
     SAYF(cCYA "afl-llvm-pass " cBRI VERSION cRST " by <lszekeres@google.com>\n");

  } else be_quiet = 1;



  if (is_insert){
     unsigned fun_id = 0;
     unsigned  bb_id = 0;	  
     for (auto &F : M){

          if (F.empty() || F.isDeclaration() || F.isIntrinsic() || isBlacklisted(&F) ||  isSourceFileInUsr(F))
              continue;
          fun_id++;
          ConstantInt *TEMP_FUNID = ConstantInt::get(Int32Ty, fun_id);
          auto meta_fun_id = MDNode::get(C, ConstantAsMetadata::get(TEMP_FUNID));
          F.setMetadata("lyso_fun_id", meta_fun_id);
          errs() << "fun id is:" << fun_id << "\n";


          for (auto &BB : F) {
              bb_id++;
              ConstantInt *TEMP_BBID = ConstantInt::get(Int32Ty, bb_id);
              auto meta_bb_id = MDNode::get(C, ConstantAsMetadata::get(TEMP_BBID));
              BB.getTerminator()->setMetadata("lyso_bb_id", meta_bb_id);
          }
     }
     return true;
  }

  if (DistanceFile.empty() || TraceFile.empty()) {
    FATAL("Provide distance file or trace file");
    return false;
  }

  std::unordered_set<unsigned> ReachedBB;
  std::unordered_set<unsigned> TraceBB;
 
  std::ifstream distancefile(DistanceFile);
  std::ifstream tracefile(TraceFile);
  std::string line;

  //Read distance file
  while (std::getline(distancefile, line)){
        std::vector<std::string> splits;
        split(splits, line, is_any_of(","));
        unsigned srcBB = stoi(splits[1]);
        ReachedBB.insert(srcBB);
  }

  //Read trace file
  while (std::getline(tracefile, line)){
        unsigned traceBB = stoi(line);
        TraceBB.insert(traceBB);
  }

  /* Decide instrumentation ratio */

  char* inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");

  }

  /* Get globals for the SHM region and the previous location. Note that
     __afl_prev_loc is thread-local. */

  GlobalVariable *AFLMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

  GlobalVariable *AFLPrevLoc = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc",
      0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  

  //This is for basic block instrumentation
  std::vector<Type*> bbParamTypes;
  bbParamTypes.push_back(Type::getInt16Ty(C));
  bbParamTypes.push_back(Type::getInt16Ty(C));
  ArrayRef<Type*> bbParamTypesRef(bbParamTypes);
  Type *bbRetType = Type::getVoidTy(C);
  FunctionType *logBBType = FunctionType::get(bbRetType, bbParamTypesRef, false);
  llvm::Function *instr_bb = Function::Create(logBBType, llvm::Function::ExternalLinkage, "__instr_bb", &M);
  
  /* Create output file */
  std::ofstream bb2bb(OutDirectory + "/BB2BB.txt", std::ofstream::out | std::ofstream::app);

  /* Instrument all the things! */
  int inst_blocks = 0;
  int new_bb_id = 0;

  for (auto &F : M){

         if ( F.empty() || F.isDeclaration() || F.isIntrinsic() || isBlacklisted(&F) ||  isSourceFileInUsr(F))
            continue;


         if (MDNode *NF = F.getMetadata("lyso_fun_id") ) {
                 
            unsigned fun_id = cast<ConstantInt>(cast<ValueAsMetadata>(NF->getOperand(0))->getValue())->getZExtValue();

            for (auto &BB : F) {
                BasicBlock::iterator IP = BB.getFirstInsertionPt();
                IRBuilder<> IRB(&(*IP));

                auto *TI = BB.getTerminator();
                if (MDNode *NBB = TI->getMetadata("lyso_bb_id")) {
                   int bb_id = cast<ConstantInt>(cast<ValueAsMetadata>(NBB->getOperand(0))->getValue())->getZExtValue();

                   /* Selective instrumentation for trace tracking*/
                   if (TraceBB.count(bb_id) > 0){
                      new_bb_id++;
                      Value *BBID = ConstantInt::get(Int32Ty, bb_id);
                      Value *NEWBBID =  ConstantInt::get(Int32Ty, new_bb_id);
                      IRB.CreateCall(instr_bb, {BBID, NEWBBID});
                      bb2bb << bb_id << "," << new_bb_id << "\n";
                   }
                   
                   if (ReachedBB.count(bb_id) > 0){
                      if (AFL_R(100) >= inst_ratio) continue;

                      /* Make up cur_loc */

                      unsigned int cur_loc = AFL_R(MAP_SIZE);

                      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

                      /* Load prev_loc */
 
                      LoadInst *PrevLoc = IRB.CreateLoad(IRB.getInt32Ty(), AFLPrevLoc);
                      PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
                      Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

                      /* Load SHM pointer */

		      LoadInst *MapPtr = IRB.CreateLoad(PointerType::get(Int8Ty, 0),AFLMapPtr);
	              MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
	              Value *MapPtrIdx =
		         IRB.CreateGEP(Int8Ty, MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

	              /* Update bitmap */

	              LoadInst *Counter = IRB.CreateLoad(IRB.getInt8Ty(), MapPtrIdx);
	              Counter->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
	              Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
	              IRB.CreateStore(Incr, MapPtrIdx)
		          ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

	              /* Set prev_loc to cur_loc >> 1 */

	              StoreInst *Store =
		          IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
	              Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

	              inst_blocks++;
                   }
               }
	  }
      }
  }
  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks) WARNF("No instrumentation targets found.");
    else OKF("Instrumented %u locations (%s mode, ratio %u%%).",
             inst_blocks, getenv("AFL_HARDEN") ? "hardened" :
             ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN")) ?
              "ASAN/MSAN" : "non-hardened"), inst_ratio);

  }

  bb2bb.close();


  return true;

}


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());

}


/* enable debug manually */
static RegisterPass<AFLCoverage> X("test", "AFL coverage instrumentation and id insertion!\n",
    false, false);


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_ModuleOptimizerEarly, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
