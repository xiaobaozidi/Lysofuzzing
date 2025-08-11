#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <set>

#include <iostream>

#include <fstream>

#include <string>

#include <sstream>

#include <list>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <queue>

#include "llvm/ADT/Statistic.h"

#include "llvm/IR/IRBuilder.h"

#include "llvm/IR/LegacyPassManager.h"

#include "llvm/IR/Module.h"

#include "llvm/Support/Debug.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Support/CommandLine.h"

#include "llvm/Support/FileSystem.h"

#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/CFGPrinter.h"

#include "cctype"

#if defined(LLVM34)
#include "llvm/DebugInfo.h"
#else
#include "llvm/IR/DebugInfo.h"
#endif

#if defined(LLVM34) || defined(LLVM35) || defined(LLVM36)
#define LLVM_OLD_DEBUG_API
#endif

using namespace llvm;

typedef std::pair < Value * , std::string > node;
typedef std::pair < node, node > edge;
typedef std::list < node > node_list;
typedef std::list < edge > edge_list;

cl::opt < std::string > TargetsFile(
  "targets",
  cl::desc("Input file containing the target lines of code."),
  cl::value_desc("targets"));

cl::opt < std::string > OutDirectory(
  "outdir",
  cl::desc("Output directory where Ftargets.txt, Fnames.txt, and BBnames.txt are generated."),
  cl::value_desc("outdir"));

std::string indent = "";
std::map< Value *, std::set < Value * >> ctrl_save;
std::map< Value *, std::set < Value * >> ictrl_save;
std::map < Value*, int> bb_num;
std::map < Value *,  float> pro_map;
std::map < BasicBlock *, std::set < BasicBlock * >> switch_map;
float min=1;
std::set < Value *> unresolvedCallSite;

static void getDebugLoc(const Instruction * I, std::string & Filename,
  unsigned & Line) {
  #ifdef LLVM_OLD_DEBUG_API
  DebugLoc Loc = I -> getDebugLoc();
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
  if (DILocation * Loc = I -> getDebugLoc()) {
    Line = Loc -> getLine();
    Filename = Loc -> getFilename().str();

    if (Filename.empty()) {
      DILocation * oDILoc = Loc -> getInlinedAt();
      if (oDILoc) {
        Line = oDILoc -> getLine();
        Filename = oDILoc -> getFilename().str();
      }
    }
  }
  #endif /* LLVM_OLD_DEBUG_API */
}

bool checkType(Function* func, CallInst* callinst) {
  int idx=0;
  bool cons=true;
  for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
    Type *parType = arg_idx->getType();
    Value * argptr = callinst -> getArgOperand(idx);
    Type *argType = argptr->getType();

    std::string par_type_str;
    raw_string_ostream rso1(par_type_str);
    parType->print(rso1);
    std::string arg_type_str;
    raw_string_ostream rso2(arg_type_str);
    argType->print(rso2);
    std::string s1 = rso1.str();
    std::string s2 = rso2.str();
    //debug << "par: " << s1 << " arg: " << s2 << "\n";

    //check 
    std::string stringPrefix="%class.";
    std::string doublePrefix="double";
    std::string struPrefix="%struct";
    //it is class pointer
    //fprintf(stderr, "before %s, %s\n", s1.c_str(), s2.c_str());
    if ( (s1.substr(0, 7)==stringPrefix && s2.substr(0, 7)!=stringPrefix)
      || (s1.substr(0, 7)!=stringPrefix && s2.substr(0, 7)==stringPrefix)) {
      cons=false;
      break;
    }
    else if ( (s1[0]=='i' && s2[0]!='i')
      || (s1[0]!='i' && s2[0]=='i')
      || (s1[0]=='i' && s2[0]=='i' && s1[0]!=s2[0])) {
      cons=false;
      break;
    }
    if ( (s1.substr(0, 6)==doublePrefix && s2.substr(0, 6)!=doublePrefix)
      || (s1.substr(0, 6)!=doublePrefix && s2.substr(0, 6)==doublePrefix)) {
      cons=false;
      break;
    }
    else if ((s1.substr(0, 7)==struPrefix && s2.substr(0, 7)!=struPrefix)
      || (s1.substr(0, 7)!=struPrefix && s2.substr(0, 7)==struPrefix)) {
      cons=false;
      break;
    }
    else if ( (s1.find("*") != std::string::npos && s1.find("*") == std::string::npos) 
      || (s1.find("*") == std::string::npos && s1.find("*") != std::string::npos)) {
      cons=false;
      break;
    }
    else if (s1[0]!=s2[0]){
      cons=false;
      break;
    }
    idx++;
    //fprintf(stderr, "after %s, %s\n", s1.c_str(), s2.c_str());
  }
  return cons;
}

bool checkType1(Function* func, InvokeInst* callinst) {
  int idx=0;
  bool cons=true;
  for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
    Type *parType = arg_idx->getType();
    Value * argptr = callinst -> getArgOperand(idx);
    Type *argType = argptr->getType();

    std::string par_type_str;
    raw_string_ostream rso1(par_type_str);
    parType->print(rso1);
    std::string arg_type_str;
    raw_string_ostream rso2(arg_type_str);
    argType->print(rso2);
    std::string s1 = rso1.str();
    std::string s2 = rso2.str();
    //debug << "par: " << s1 << " arg: " << s2 << "\n";

    //check 
    std::string stringPrefix="%class.";
    std::string doublePrefix="double";
    std::string struPrefix="%struct";
    //it is class pointer
    //fprintf(stderr, "before %s, %s\n", s1.c_str(), s2.c_str());
    if (s1.substr(0, 7)==stringPrefix && s2.substr(0, 7)!=stringPrefix
      || s1.substr(0, 7)!=stringPrefix && s2.substr(0, 7)==stringPrefix) {
      cons=false;
      break;
    }
    else if (s1[0]=='i' && s2[0]!='i'
      || s1[0]!='i' && s2[0]=='i'
      || s1[0]=='i' && s2[0]=='i' && s1[0]!=s2[0]) {
      cons=false;
      break;
    }
    if (s1.substr(0, 6)==doublePrefix && s2.substr(0, 6)!=doublePrefix
      || s1.substr(0, 6)!=doublePrefix && s2.substr(0, 6)==doublePrefix) {
      cons=false;
      break;
    }
    else if (s1.substr(0, 7)==struPrefix && s2.substr(0, 7)!=struPrefix
      || s1.substr(0, 7)!=struPrefix && s2.substr(0, 7)==struPrefix) {
      cons=false;
      break;
    }
    else if (s1.find("*") != std::string::npos && s1.find("*") == std::string::npos 
      || s1.find("*") == std::string::npos && s1.find("*") != std::string::npos) {
      cons=false;
      break;
    }
    else if (s1[0]!=s2[0]){
      cons=false;
      break;
    }
    idx++;
    //fprintf(stderr, "after %s, %s\n", s1.c_str(), s2.c_str());
  }
  return cons;
}

static unsigned int num = 0;

static std::string LLVMInstructionAsString(Instruction * I) {
  std::string instString;
  raw_string_ostream N(instString);
  I -> print(N);
  return N.str();
}

static std::string getvaluestaticname(Value * val) {
  std::string ret_val = "val";
  /*
  if (val -> getName().empty()) {
    ret_val += std::to_string(num);
    num++;
  } else
    ret_val = val -> getName().str();

  //val.nodeID.instruction
  if (isa < Instruction > (val))
    ret_val += ":" + LLVMInstructionAsString(dyn_cast < Instruction > (val));
  */
  return ret_val;
}

static std::string getvaluestaticname1(Value * val) {
  std::string ret_val = "val";
  
  if (val -> getName().empty()) {
    ret_val += std::to_string(num);
    num++;
  } else
    ret_val = val -> getName().str();

  //val.nodeID.instruction
  if (isa < Instruction > (val))
    ret_val += ":" + LLVMInstructionAsString(dyn_cast < Instruction > (val));
  
  return ret_val;
}

//find the pro of val
void find_pro (Value *val, std::vector < Value * > targetIns, std::set <Value *> in_set) {
  //have analyzed it
  if (pro_map.count(val)==1) {
    return;
  }
  //it is target ins
  auto it = find(targetIns.begin(), targetIns.end(), val);
  if (it != targetIns.end())
  {
    pro_map[val]=1;
    return;
  }
  //it is the parent ins
  bool is_in = in_set.find(val) != in_set.end();
  if(is_in) {
    pro_map[val]=0;
    return;
  }
  //ignore unresolved call sites
  is_in = unresolvedCallSite.find(val) != unresolvedCallSite.end();
  if (is_in) {
    pro_map[val]=0;
    return;
  }
  //calculate its children first
  if(ctrl_save.count(val) == 1) {
    float ret=0;
    int num=1;
    std::set <Value *> tmp(in_set);
    tmp.insert(val);
    //get its children
    for (auto ins: ctrl_save[val]) {
      find_pro(ins, targetIns, tmp);
      //it is not an indirect call
      if(ictrl_save.count(val) ==0 )
        ret = ret+pro_map[ins];
      //it is an indirect call
      else {
        int callNum = ictrl_save[val].size();
        ret = ret+pro_map[ins]/callNum;
      }
    }
    if (isa < Instruction > (val)) {
        Instruction * ins = dyn_cast < Instruction > (val);
        if(ins -> getOpcode()==Instruction::Br) {
          BranchInst *brins = dyn_cast < BranchInst > (ins);
          num = brins->getNumSuccessors();
        }
        else if(ins -> getOpcode()==Instruction::Switch) {
          SwitchInst *swins = dyn_cast < SwitchInst > (ins);
          num = swins->getNumSuccessors();
          BasicBlock * B = ins -> getParent();
          for (succ_iterator SI = succ_begin(B), E = succ_end(B); SI != E; ++SI) {
            BasicBlock * Succ = * SI;
            switch_map[B].insert(Succ);
          }
        }
        else if(ins -> getOpcode()==Instruction::IndirectBr) {
          IndirectBrInst *indbrins = dyn_cast < IndirectBrInst > (ins);
          num = indbrins->getNumSuccessors();
        }
    }
    if(num==0) pro_map[val]=0;
    else pro_map[val]=ret/num;
  }
  else {
    pro_map[val]=0;
  }
}

//get file name of targer function
std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
            break;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    result.pop_back();
    return result;
}

std::string getSpecLine(std::string file, std::string line) {
    std::ifstream realFile(file);
    int l=0;
    int lineT = atoi(line.c_str());
    std::string line1;
    while (l<lineT&&std::getline(realFile, line1)) {
      l++;
    }
    realFile.close();
    return line1;
}

namespace {

  struct DFUZZPASS: public ModulePass {

    public:

      static char ID;

    std::list < std::string > targets;
    std::list < std::string > reals;
    edge_list data_flow_edges;
    std::list < node > globals;
    std::map < Value * ,
    Function * > func_calls;
    std::map < Value * ,
    std::set <Function *> > func_icalls;
    std::map < Function * ,
    node_list > func_args;
    std::map < Function * ,
    node_list > func_nodes_ctrl;
    std::map < Function * ,
    edge_list > func_edges_ctrl;
    std::set < Value * > brIns;
    std::vector < Value * > targetIns;
    std::multimap < Value * ,
    Value * > ctrl_rel;
    std::multimap < Value * ,
    Value * > data_rel;
    std::set < Value * > tar_ins;
    std::set < Value * > all_bb;
    std::map < Function * ,
    node_list > func_rets;
    std::map < Value * ,
    Value * > ret_call;
    std::set < Value * > rel_ins;
    std::set < BasicBlock * > rel_bb;
    std::map < BasicBlock * ,
    std::string > map_bb;
    std::map < std::string, Instruction * > indirectCallSiteLoc;
    std::map < std::string, Instruction * > invokeLoc;
    std::map < std::string, Function * > functionLoc;
    std::set < std::string > functionStart;
    std::map < std::string, Function * > functionAbsLoc;
    std::map < std::string, std::set < std::string> > indirectSave;
    std::map < std::string, Function * > functionMap; 
    std::set < Function* > functionSet;
    std::set < Function* > hasAnaFunctionSet;
    std::set < Function* > hasCallsite;
    std::map < Function*, std::set<Value* >> funcToCall;
  
    DFUZZPASS(): ModulePass(ID) {
      ID = 0;
    }

    //bool runOnModule(Module & M) override;

    // StringRef getPassName() const override {
    //  return "American Fuzzy Lop Instrumentation";
    // }

    std::string remove_special_chars(std::string in_str) {
      std::string ret_val = in_str;
      size_t pos;
      while ((pos = ret_val.find('.')) != std::string::npos) {
        ret_val[pos] = '_';
      }

      return ret_val;
    }

    bool dumpNodes(std::ofstream & Out, llvm::Function & F)  {
      //for (auto node_l: func_nodes_ctrl[ & F])
        //Out << indent << "\tNode" << node_l.first << "[shape=record, label=\"" << node_l.second << "\"];\n";
      return false;
    } 

    bool dumpControlflowEdges(std::ofstream & Out, llvm::Function & F)  {
      for (auto edge_l: func_edges_ctrl[ & F]) {
        //Out << indent << "\tNode" << edge_l.first.first << " -> Node" << edge_l.second.first << ";\n";
        ctrl_rel.insert(std::make_pair(edge_l.second.first, edge_l.first.first));
        ctrl_save[edge_l.first.first].insert(edge_l.second.first);
      }
      return false;
    }

    bool dumpGlobals(std::ofstream & Out)  {
      //for (auto globalval: globals)
        //Out << indent << "\tNode" << globalval.first << "[shape=record, label=\"" << globalval.second << "\"];\n";
      return false;
    }

    bool dumpDataflowEdges(std::ofstream & Out) {
      for (auto edge_l: data_flow_edges) {
        //Out << indent << "\tNode" << edge_l.first.first << " -> Node" << edge_l.second.first << "[color=red];\n";
        data_rel.insert(std::make_pair(edge_l.second.first, edge_l.first.first));
      }
      return false;
    }

    bool dumpFunctionCalls(std::ofstream & Out)  {
      for (auto call_l: func_calls) {
        if ((call_l.second) != NULL) {
          funcToCall[call_l.second].insert(call_l.first);
          //Out << indent << "\tNode" << &*(call_l.second->front().begin()) << " -> Node"<< call_l.first <<"[ltail = cluster_"<< remove_special_chars(call_l.second->getName().str())<<", color=red, label=return];\n";
          for (auto & BB: * call_l.second) {
            for (auto & II: BB) {
              ctrl_rel.insert(std::make_pair(&II, call_l.first));
              ctrl_save[call_l.first].insert(&II);
              break;
            }
            break;
          }  
        }
      }

      for (auto call_l: func_icalls) {
          for(Function* tar: call_l.second){
            if (tar != NULL) {
              funcToCall[tar].insert(call_l.first);
              //Out << indent << "\tNode" << &*(call_l.second->front().begin()) << " -> Node"<< call_l.first <<"[ltail = cluster_"<< remove_special_chars(call_l.second->getName().str())<<", color=red, label=return];\n";
              for (auto & BB: *tar) {
                for (auto & II: BB) {
                  ctrl_rel.insert(std::make_pair(&II, call_l.first));
                  ctrl_save[call_l.first].insert(&II);
                  ictrl_save[call_l.first].insert(&II);
                  break;
                }
                break;
              }  
            }
          }
      }
      for (auto ret_cal: ret_call) {
        //Out << indent << "\tNode" << ret_cal.first << vim  ./libming-CVE-2016-9827/obj-aflgo/temp/distance.cfg.txt" -> Node" << ret_cal.second << "[color=red, label=return];\n";
        data_rel.insert(std::make_pair(ret_cal.second, ret_cal.first));
      }
      return false;
    }

    bool dumpFunctionArguments(std::ofstream & Out, Function & F)  {
      ///TODO:Argument in graphviz struct style. i.e., as collection of nodes
      //for (auto arg_l: func_args[ & F])
        //Out << indent << "\tNode" << arg_l.first << "[label=" << arg_l.second << ", shape=doublecircle, style=filled, color=blue , fillcolor=red];\n";
      return false;
    } 

    bool runOnModule(Module & M) {

       int total_BB=0;
       std::ofstream cfg_ddg(OutDirectory + "/ctrl-data.dot", std::ofstream::out | std::ofstream::app);
       std::ofstream debug(OutDirectory + "/debug.txt", std::ofstream::out | std::ofstream::app);
       std::ofstream distance(OutDirectory + "/distance.cfg.txt", std::ofstream::out | std::ofstream::app);

      if (!TargetsFile.empty()) {

        if (OutDirectory.empty()) {
          //FATAL("Provide output directory '-outdir <directory>'");
          return false;
        }

        std::ifstream targetsfile(TargetsFile);
        std::string line;
        while (std::getline(targetsfile, line))
          targets.push_back(line);
        targetsfile.close();

        std::ifstream realFile(OutDirectory + "/real.txt");
        std::string line1;
        while (std::getline(realFile, line1)) {
          reals.push_back(line1);
          //debug << "real: " << line1 << "\n";
        }
        realFile.close();
      }

      
      //cfg_ddg << "module: " << M.getSourceFileName() << "\n";
      //cfg_ddg << "targets: " << targets.front() << "\n";
      //debug << "init " << "\n";
      fprintf(stderr, "init");
      int count = 0;
      for (auto FI = M.getFunctionList().begin(), FE = M.getFunctionList().end(); FI != FE; ++FI) {

        Function* Fun = dyn_cast<Function> (FI);
        functionMap[Fun->getName().str()] = Fun;
        functionSet.insert(Fun);
        debug << Fun->getName().str() << " " << Fun->arg_size() << "\n";
        if (DISubprogram *SP = Fun->getSubprogram()) {
          if (SP->describes(Fun)) {
           std::string filename = SP->getFilename().str();
           std::string absloc = filename+","+ std::to_string(SP->getLine());
           functionAbsLoc[absloc] = Fun;
           std::size_t found = filename.find_last_of("/\\");
           if (found != std::string::npos)
              filename = filename.substr(found + 1);
           //rawstr << filename << "," << SP->getLine();
           std::string loc = filename+","+ std::to_string(SP->getLine());
           //functionLoc.insert(std::pair<std::string, Function*> (loc, Fun));
           functionLoc[loc] = Fun;
          }
        }
        count++;
        bool is_target = false;
        bool newFun = true;
        fprintf(stderr,  "@ %d, %d", count, M.getFunctionList().size());
        //each block
        for (auto BB = FI -> getBasicBlockList().begin(), BE = FI -> getBasicBlockList().end(); BB != BE; ++BB) {
          total_BB++;
          BasicBlock * BT = dyn_cast < BasicBlock > (BB);
          all_bb.insert(BT);
          std::string bb_name("");
          std::string filename;
          unsigned line;
          //each instruction
          for (auto II = BB -> begin(), IE = BB -> end(); II != IE; ++II) {
            Instruction * IT = dyn_cast < Instruction > (II);;
            getDebugLoc( IT, filename, line);
            static const std::string Xlibs("/usr/");
            //debug << filename << ":" << line << "\n";
            //cfg_ddg << indent << "line: " << line << "\n";
            if (newFun) {
              functionStart.insert(filename+":"+std::to_string(line));
              newFun=false;
              brIns.insert(IT);
            }
            if (!(filename.empty() || line == 0 || !filename.compare(0, Xlibs.size(), Xlibs))) {
              std::size_t found = filename.find_last_of("/\\");
              if (found != std::string::npos)
                filename = filename.substr(found + 1);

              if(bb_name.empty()) {
                bb_name = filename + ":" + std::to_string(line);
                map_bb.insert( std::pair<BasicBlock*, std::string> (BT, bb_name));
              }

              //save target instructions
              for (auto & real: reals) {
                std::size_t found = real.find_last_of("/\\");
                if (found != std::string::npos)
                  real = real.substr(found + 1);

                std::size_t pos = real.find_last_of(":");
                std::string real_file = real.substr(0, pos);
                unsigned int real_line = atoi(real.substr(pos + 1).c_str());

                //cfg_ddg << indent << target_file << filename << target_line << line << "\n";

                if (!real_file.compare(filename) && real_line == line) {
                  targetIns.push_back( IT);
                  debug << "real: " << filename << line << "\n";
                }

              }
                  

              if (!is_target) {
                for (auto & target: targets) {
                  std::size_t found = target.find_last_of("/\\");
                  if (found != std::string::npos)
                    target = target.substr(found + 1);

                  std::size_t pos = target.find_last_of(":");
                  std::string target_file = target.substr(0, pos);
                  unsigned int target_line = atoi(target.substr(pos + 1).c_str());

                  //cfg_ddg << indent << target_file << filename << target_line << line << "\n";

                  if (!target_file.compare(filename) && target_line == line) {
                    is_target = true;
                  }

                }
              }
            }

            
            //analyze each instruction
            switch (II -> getOpcode()) {
            case Instruction::Invoke: {
              std::string filename1;
              unsigned line1;
              getDebugLoc( IT, filename1, line1);
              static const std::string Xlibs("/usr/");
              //cfg_ddg << indent << "line: " << line << "\n";
              if (!(filename1.empty() || line1 == 0 || !filename1.compare(0, Xlibs.size(), Xlibs))) {
                std::string callerLoc = filename1+","+std::to_string(line1);
                invokeLoc[callerLoc] = IT;
              }
              break;
            }
            case Instruction::Call: {
              //debug << "callsite: " <<  filename << ":" << line << "\n";
              CallInst * callinst = dyn_cast < CallInst > (II);
              Function * func = callinst -> getCalledFunction();
              //if (func!=NULL) debug << "1: " << func->getName().str() << "\n";
              //if (func -> isDeclaration()) debug << func->getName().str() << "\n";
              //else if (func == NULL) debug << "2: " << "\n";

              //it is an indirect call or lib func
              if (func == NULL || func -> isDeclaration()) {
                int num = callinst -> getNumArgOperands();
                
                //the call site is an indirect call
                if(func==NULL) {
                  std::string filename1;
                  unsigned line1;
                  getDebugLoc( IT, filename1, line1);
                  static const std::string Xlibs("/usr/");
                  //cfg_ddg << indent << "line: " << line << "\n";
                  if (!(filename1.empty() || line1 == 0 || !filename1.compare(0, Xlibs.size(), Xlibs))) {
                    //std::size_t found = filename1.find_last_of("/\\");
                    //if (found != std::string::npos)
                    //  filename1 = filename1.substr(found + 1);

                    std::string callerLoc = filename1+","+std::to_string(line1);
                    //indirectCallSiteLoc.insert(std::pair<std::string, Function*> (callerLoc, IT));
                    indirectCallSiteLoc[callerLoc] = IT;
                  }
                }
                //the called function is declared only
                else if(func->isDeclaration()) {
                  for (int idx = 0; idx < num; idx++) {
                    Value * argptr = callinst -> getArgOperand(idx);
                    data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * II, getvaluestaticname( & * II))));
                  }
                }

              }
              //user-defined functions
              else {
                func_calls[ & * II] = func;
                hasCallsite.insert(func);
                int num = callinst -> getNumArgOperands();
                //it is a direct call
                int idx = 0;
                for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
                  if (idx>=num) break;
                  Value * argptr = callinst -> getArgOperand(idx);
                  idx++;
                  hasAnaFunctionSet.insert(func);
                  func_args[func].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
                  data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
                  //data_flow_edges.push_back(edge(node(&*II, getvaluestaticname(&*II)), node(&*arg_idx, getvaluestaticname(&*arg_idx))));
                }
              }
              break;
            }
            case Instruction::Store: {
              StoreInst * storeinst = dyn_cast < StoreInst > (II);
              Value * storevalptr = storeinst -> getPointerOperand();
              Value * storeval = storeinst -> getValueOperand();
              data_flow_edges.push_back(edge(node( & * II, getvaluestaticname( & * II)), node(storevalptr, getvaluestaticname(storevalptr))));
              data_flow_edges.push_back(edge(node(storeval, getvaluestaticname(storeval)), node( & * II, getvaluestaticname( & * II))));
              break;
            }
            case Instruction::Load: {
              LoadInst * loadinst = dyn_cast < LoadInst > (II);
              Value * loadvalptr = loadinst -> getPointerOperand();
              data_flow_edges.push_back(edge(node(loadvalptr, getvaluestaticname(loadvalptr)), node( & * II, getvaluestaticname(loadvalptr))));
              break;
            }
            case Instruction::Ret: {
              func_rets[ & * FI].push_back(node( & * II, getvaluestaticname( & * II)));
            }
            default: {
              for (Instruction::op_iterator op = II -> op_begin(), ope = II -> op_end(); op != ope; ++op) {
                if (dyn_cast < Instruction > ( * op) || dyn_cast < Argument > ( * op)) {
                  data_flow_edges.push_back(edge(node(op -> get(), getvaluestaticname(op -> get())), node( & * II, getvaluestaticname( & * II))));
                }
              }
              break;
            }
            }

            BasicBlock::iterator previous = II;
            ++previous;
            func_nodes_ctrl[ & * FI].push_back(node( & * II, getvaluestaticname( & * II)));
            if (previous != IE)
              func_edges_ctrl[ & * FI].push_back(edge(node( & * II, getvaluestaticname( & * II)), node( & * previous, getvaluestaticname( & * previous))));

          }

          //connect this block to its succ blocks
          Instruction * TI = BB -> getTerminator();

          int succNum = 0;
          BasicBlock * B = TI -> getParent();
          for (succ_iterator SI = succ_begin(B), E = succ_end(B); SI != E; ++SI) {
            succNum++;
            BasicBlock * Succ = * SI;
            Value * succ_inst = & * (Succ -> begin());
            func_edges_ctrl[ & * FI].push_back(edge(node(TI, getvaluestaticname(TI)), node(succ_inst, getvaluestaticname(succ_inst))));
          }

          /*
          if(TI -> getOpcode()==Instruction::Br) {
            brIns.insert(TI);
          }
          else if (TI -> getOpcode()==Instruction::Switch) {
            brIns.insert(TI);
          }
          //more than one succ
          if (succNum > 1) {
            bb_num[TI] = succNum;
            brIns.insert(TI);
          }
          */
        }
      }

      //debug << "done first pass " << "\n";
      fprintf(stderr, "done first pass");
      debug << "total functions: " << functionSet.size() << "\n"; 

      //resolve indirect call site
      std::ifstream indirectFile(OutDirectory + "/indirect.txt");
      std::string iline;
      while (std::getline(indirectFile, iline)) {
        int num = std::count(iline.begin(),iline.end(),' ');
        if(num==1) {
          std::size_t pos = iline.find(' ');
          std::string callSite = iline.substr(0, pos);
          std::string Func = iline.substr(pos+1);
          //indirectSave.insert(std::pair<std::string, std::string> (callSite, Func));
          indirectSave[callSite].insert(Func);
        }
      }
      std::ifstream IIcalls(OutDirectory + "/IIcalls.txt");
      std::string iiline;
      std::map<std::string, std::set<std::string>> line2fun;
      while (std::getline(IIcalls, iiline)) {
        int num = std::count(iiline.begin(),iiline.end(),' ');
        if(num==1) {
          std::size_t pos = iiline.find(' ');
          std::string callSite = iiline.substr(0, pos);
          std::string Func = iiline.substr(pos+1);
          line2fun[callSite].insert(Func);
        }
      }

      //cannot resolve using pointer-to analysis and IIcalls
      std::map < std::string, Instruction * > unresolved;
      //cannot resolve using function name matching
      std::map < std::string, Instruction * > unresolved1;

      std::map < std::string, Instruction * > unresolvedivk;
      //cannot resolve using function name matching
      std::map < std::string, Instruction * > unresolvedivk1;

      for(auto ind: indirectCallSiteLoc) {
        std::string callSiteAbsLoc = ind.first;
        std::string callSiteLoc = callSiteAbsLoc;
        std::size_t found = callSiteAbsLoc.find_last_of("/\\");
        if (found != std::string::npos)
          callSiteLoc = callSiteLoc.substr(found + 1);
        Instruction *II = ind.second;
        CallInst *callinst = dyn_cast < CallInst > (II);
        //we can resolve this indirect call using wpa
        if (indirectSave.count(callSiteLoc) != 0) {
          std::set<std::string> funcs = indirectSave[callSiteLoc];
          for(std::string func: funcs) {
            if (functionLoc.count(func) != 0) {
              Function* funcIns = functionLoc[func];
              func_icalls[callinst].insert(funcIns);
              hasCallsite.insert(funcIns);
              int num = callinst -> getNumArgOperands();
              int idx = 0;
              for (auto arg_idx = funcIns -> arg_begin(), arg_end = funcIns -> arg_end(); arg_idx != arg_end; ++arg_idx) {
                if(idx>=num) break;
                Value * argptr = callinst -> getArgOperand(idx);
                idx++;
                hasAnaFunctionSet.insert(funcIns);
                func_args[funcIns].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
                data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
                //data_flow_edges.push_back(edge(node(&*II, getvaluestaticname(&*II)), node(&*arg_idx, getvaluestaticname(&*arg_idx))));
              }
            }
          }
        }
        //we can get the target function from IIcalls.txt
        else if(line2fun.count(callSiteLoc) !=0 ) {
          std::set<std::string> funcs = line2fun[callSiteLoc];
          for(std::string func: funcs) {
            if (functionMap.count(func) != 0) {
              Function* funcIns = functionMap[func];
              int idx = 0;
              if(funcIns->arg_size() != callinst->getNumArgOperands())
              {
                continue;
              }
              bool cons = true;
              //check type
              cons = checkType(funcIns, callinst);
              if (cons==false) {
                continue;
              }
              func_icalls[callinst].insert(funcIns);
              hasCallsite.insert(funcIns);
              for (auto arg_idx = funcIns -> arg_begin(), arg_end = funcIns -> arg_end(); arg_idx != arg_end; ++arg_idx) {
                //default value
                if(idx>=callinst->getNumArgOperands()){
                  break;
                }
                Value * argptr = callinst -> getArgOperand(idx);
                idx++;
                hasAnaFunctionSet.insert(funcIns);
                func_args[funcIns].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
                data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
                //data_flow_edges.push_back(edge(node(&*II, getvaluestaticname(&*II)), node(&*arg_idx, getvaluestaticname(&*arg_idx))));
              }
            }
          }
        }
        //we cannot analyze the target functions of this call site :(
        else {
          unresolved[callSiteAbsLoc] = II;
          //unresolvedCallSite.insert(II);
        }
      }

      //resolve invoke instructions
      for(auto ins: invokeLoc) {
        Instruction *II = ins.second;
        InvokeInst *invokeInst = dyn_cast < InvokeInst > (II);
        Function *func = invokeInst->getCalledFunction();
        if(func!=NULL) {
          int idx=0;
          if(func->arg_size() != invokeInst->getNumArgOperands())
          {
            continue;
          }
          debug << "invoke function: " << ins.first << " " << func->getName().str() << "\n";
          func_calls[invokeInst]=func;
          for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
            Value * argptr = invokeInst -> getArgOperand(idx);
            idx++;
            hasAnaFunctionSet.insert(func);
            func_args[func].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
            data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
          }
        }
        else {
          unresolvedivk[ins.first] = II;
        }
      }

      //debug << "unresolved functions " << functionSet.size() << "\n";
      
      //matching function name
      
      for (auto ind: unresolved) {
        std::string callSiteLoc = ind.first;
        std::size_t pos = callSiteLoc.find(',');
        std::string callSiteFile = callSiteLoc.substr(0, pos);
        std::string line = callSiteLoc.substr(pos+1);
        //can we analyze this call site?
        bool flag = false;
        std::string callsiteLine = getSpecLine(callSiteFile, line);
        //this is not a call site
        if (callsiteLine.find("(") == std::string::npos) {
            continue;
        }
        //debug << "command " << command1 << " " << tarFile << " " << line << " " << callsiteLine << "\n";
        Instruction *II = ind.second;
        CallInst *callinst = dyn_cast < CallInst > (II);

        //analyze this callsite
        for(auto pair: functionAbsLoc) {
            std::string Loc = pair.first;
            pos = Loc.find(',');
            std::string FunctionFile = Loc.substr(0, pos);
            std::string Functionline = Loc.substr(pos+1);
            std::string functionName = getSpecLine(FunctionFile, Functionline);
            //this is not a function
            if (functionName.find("(") == std::string::npos) {
              continue;
            }
            std::size_t found = functionName.find_last_of("(");
            functionName = functionName.substr(0, found+1);
            found = functionName.find_last_of(" ");
            functionName = functionName.substr(found+1);
            if (functionName.find("::") != std::string::npos) {
              found = functionName.find_last_of("::");
              functionName = functionName.substr(found+1);
            }
            if(functionName[0] == '*') {
              functionName = functionName.substr(1);
            }
            //this is not a function name
            if(isalpha(functionName[0])==0 && functionName[0]!='_') {
              continue;
            }
            //debug << callsiteLine << " " << functionName << "\n";
            //debug << "command " << FunctionFile << " " << Functionline << " " << functionName << "\n";
            //debug << callsiteLine << " " << functionName << "\n";
            //it is a method call and the function name is inconsistant
            if (callsiteLine.find(functionName) == std::string::npos){
              continue;
            }
            //debug << "command " << FunctionFile << " " << Functionline << " " << functionName << "\n";
            Function* func = pair.second;
            int idx = 0;
            //we consider default argument values
            if(func->arg_size() != callinst->getNumArgOperands())
            {
              continue;
            }
            bool cons = true;
            //check type
            cons = checkType(func, callinst);
            if (cons==false) {
              continue;
            }
            //debug << "valid " << callsiteLine << " " << functionName << "\n";
            flag = true;
            func_icalls[callinst].insert(func);
            for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
              if(idx>=callinst->getNumArgOperands()){
                break;
              }
              Value * argptr = callinst -> getArgOperand(idx);
              idx++;
              hasAnaFunctionSet.insert(func);
              func_args[func].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
              data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
              //data_flow_edges.push_back(edge(node(&*II, getvaluestaticname(&*II)), node(&*arg_idx, getvaluestaticname(&*arg_idx))));
            }
        }
        

        //we cannot analyze this call site
        if (flag==false)
          //we only include call sites containing "*",
          //we only handle call sites containing ->, because wpa handles C indirect calls.
          if(callsiteLine.find("->") != std::string::npos)
            unresolved1[callSiteLoc] = II;
      }

      for (auto ind: unresolvedivk) {
        std::string callSiteLoc = ind.first;
        std::size_t pos = callSiteLoc.find(',');
        std::string callSiteFile = callSiteLoc.substr(0, pos);
        std::string line = callSiteLoc.substr(pos+1);
        //can we analyze this call site?
        bool flag = false;
        std::string callsiteLine = getSpecLine(callSiteFile, line);
        //this is not a call site
        if (callsiteLine.find("(") == std::string::npos) {
            continue;
        }
        //debug << "command " << command1 << " " << tarFile << " " << line << " " << callsiteLine << "\n";
        Instruction *II = ind.second;
        InvokeInst *callinst = dyn_cast < InvokeInst > (II);

        //analyze this callsite
        for(auto pair: functionAbsLoc) {
            std::string Loc = pair.first;
            pos = Loc.find(',');
            std::string FunctionFile = Loc.substr(0, pos);
            std::string Functionline = Loc.substr(pos+1);
            std::string functionName = getSpecLine(FunctionFile, Functionline);
            //this is not a function
            if (functionName.find("(") == std::string::npos) {
              continue;
            }
            std::size_t found = functionName.find_last_of("(");
            functionName = functionName.substr(0, found+1);
            found = functionName.find_last_of(" ");
            functionName = functionName.substr(found+1);
            if (functionName.find("::") != std::string::npos) {
              found = functionName.find_last_of("::");
              functionName = functionName.substr(found+1);
            }
            if(functionName[0] == '*') {
              functionName = functionName.substr(1);
            }
            //this is not a function name
            if(isalpha(functionName[0])==0 && functionName[0]!='_') {
              continue;
            }
            //debug << callsiteLine << " " << functionName << "\n";
            //debug << "command " << FunctionFile << " " << Functionline << " " << functionName << "\n";
            //debug << callsiteLine << " " << functionName << "\n";
            //it is a method call and the function name is inconsistant
            if (callsiteLine.find(functionName) == std::string::npos){
              continue;
            }
            //debug << "command " << FunctionFile << " " << Functionline << " " << functionName << "\n";
            Function* func = pair.second;
            int idx = 0;
            //we consider default argument values
            if(func->arg_size() != callinst->getNumArgOperands())
            {
              continue;
            }
            bool cons = true;
            //check type
            cons = checkType1(func, callinst);
            if (cons==false) {
              continue;
            }
            //debug << "valid " << callsiteLine << " " << functionName << "\n";
            flag = true;
            func_icalls[callinst].insert(func);
            for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
              if(idx>=callinst->getNumArgOperands()){
                break;
              }
              Value * argptr = callinst -> getArgOperand(idx);
              idx++;
              hasAnaFunctionSet.insert(func);
              func_args[func].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
              data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
              //data_flow_edges.push_back(edge(node(&*II, getvaluestaticname(&*II)), node(&*arg_idx, getvaluestaticname(&*arg_idx))));
            }
        }
        

        //we cannot analyze this call site
        if (flag==false)
          //we only include call sites containing "*",
          //we only handle call sites containing ->, because wpa handles C indirect calls.
          if(callsiteLine.find("->") != std::string::npos)
            unresolvedivk1[callSiteLoc] = II;
      }
      


      //analyze the rest call sites
      for(auto pair: unresolved1) {
        std::string callSiteLoc = pair.first;
        debug << "rest call site " << callSiteLoc << "\n";
        Instruction *II = pair.second;
        CallInst *callinst = dyn_cast < CallInst > (II);
        for(Function* func: functionSet) {
          //this function has other call sites, we do not consider it.
          if(hasAnaFunctionSet.count(func)!=0) {
            continue;
          }
          if(func->arg_size() != callinst->getNumArgOperands())
          {
            continue;
          }
          bool cons = true;
          //check type
          cons = checkType(func, callinst);
          if (cons==false) {
            continue;
          }
          //data flow
          int idx=0;
          func_icalls[callinst].insert(func);
          for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
            if(idx>=callinst->getNumArgOperands()){
              break;
            }
            Value * argptr = callinst -> getArgOperand(idx);
            idx++;
            func_args[func].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
            data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
            //data_flow_edges.push_back(edge(node(&*II, getvaluestaticname(&*II)), node(&*arg_idx, getvaluestaticname(&*arg_idx))));
          }
        }
      }

      /*
      //analyze the rest call sites
      for(auto pair: unresolvedivk1) {
        std::string callSiteLoc = pair.first;
        debug << "rest call site " << callSiteLoc << "\n";
        Instruction *II = pair.second;
        InvokeInst *callinst = dyn_cast < InvokeInst > (II);
        for(Function* func: functionSet) {
          //this function has other call sites, we do not consider it.
          if(hasAnaFunctionSet.count(func)!=0) {
            continue;
          }
          if(func->arg_size() != callinst->getNumArgOperands())
          {
            continue;
          }
          bool cons = true;
          //check type
          cons = checkType1(func, callinst);
          if (cons==false) {
            continue;
          }
          //data flow
          int idx=0;
          func_icalls[callinst].insert(func);
          for (auto arg_idx = func -> arg_begin(), arg_end = func -> arg_end(); arg_idx != arg_end; ++arg_idx) {
            if(idx>=callinst->getNumArgOperands()){
              break;
            }
            Value * argptr = callinst -> getArgOperand(idx);
            idx++;
            func_args[func].push_back(node( & * arg_idx, getvaluestaticname( & * arg_idx)));
            data_flow_edges.push_back(edge(node(argptr, getvaluestaticname(argptr)), node( & * arg_idx, getvaluestaticname( & * arg_idx))));
            //data_flow_edges.push_back(edge(node(&*II, getvaluestaticname(&*II)), node(&*arg_idx, getvaluestaticname(&*arg_idx))));
          }
        }
      }
      */

      //ret -> call ins
      for (auto call_l: func_calls) {
        //get the called function
        Function * calledFun = call_l.second;
        Value * callins = call_l.first;
        //get the return values of called function
        for (auto rets: func_rets[calledFun]) {
          Value * retValue = rets.first;
          ret_call[retValue] = callins;
        }
      }
      for (auto call_l: func_icalls) {
        //get the called function
        for (Function* tar: call_l.second) {
          //Function * calledFun = call_l.second;
          Value * callins = call_l.first;
          //get the return values of called function
          for (auto rets: func_rets[tar]) {
            Value * retValue = rets.first;
            ret_call[retValue] = callins;
          }
        }
      }

      //cfg_ddg << indent << "digraph \"control_and_data_flow\"{\n";
      indent = "\t";
      //cfg_ddg << indent << "compound=true;\n";
      //cfg_ddg << indent << "nodesep=1.0;\n";
      //cfg_ddg << indent << "rankdir=TB;\n";
      //cfg_ddg << indent << "subgraph cluster_globals{\n";
      //cfg_ddg << indent << "label=globalvaldefinitions;\n";
      //cfg_ddg << indent << "color=green;\n";
      //if (dumpGlobals(cfg_ddg)) return true;
      //cfg_ddg << indent << "}\n";
      for (auto func_node_pair: func_nodes_ctrl) {
        indent = "\t";
        //dumpFunctionArguments(cfg_ddg, * func_node_pair.first);
        //cfg_ddg << indent << "subgraph cluster_" << func_node_pair.first -> getName().str() << "{\n";
        indent = "\t\t";
        //cfg_ddg << indent << "label=\"" << func_node_pair.first -> getName().str() << "\";\n";
        //cfg_ddg << indent << "color=blue;\n";
        //dumpNodes(cfg_ddg, * func_node_pair.first);
        dumpControlflowEdges(cfg_ddg, * func_node_pair.first);
        indent = "\t";
        //cfg_ddg << indent << "}\n\n";
      }
      dumpDataflowEdges(cfg_ddg);
      dumpFunctionCalls(cfg_ddg);
      //cfg_ddg << indent << "label=control_and_data_flow_graph;\n";
      indent = "";
      //cfg_ddg << indent << "}\n";
      int BBID = 0;

      //debug << "done dump" << "\n";
      fprintf(stderr, "done dump");

      /*
      count = 0;
      for (auto & F: M) {
        count++;
        fprintf(stderr,  "@@ %d, %d", count, M.getFunctionList().size());
        bool is_target = false;
        for (auto & BB: F) {
          //cfg_ddg << "BB name: " << & BB << "\n";
          //map_bb.insert( std::pair<BasicBlock*, int> (& BB, BBID));
          //BBID++;
          std::string bb_name("");
          std::string filename;
          unsigned line;
          for (auto & I: BB) {
            getDebugLoc( & I, filename, line);
            static const std::string Xlibs("/usr/");
            //cfg_ddg << indent << "line: " << line << "\n";
            if (filename.empty() || line == 0 || !filename.compare(0, Xlibs.size(), Xlibs))
              continue;

            std::size_t found = filename.find_last_of("/\\");
            if (found != std::string::npos)
              filename = filename.substr(found + 1);

            if(bb_name.empty()) {
              bb_name = filename + ":" + std::to_string(line);
              map_bb.insert( std::pair<BasicBlock*, std::string> (& BB, bb_name));
            }

            //save target instructions
            for (auto & real: reals) {
                std::size_t found = real.find_last_of("/\\");
                if (found != std::string::npos)
                  real = real.substr(found + 1);

                std::size_t pos = real.find_last_of(":");
                std::string real_file = real.substr(0, pos);
                unsigned int real_line = atoi(real.substr(pos + 1).c_str());

                //cfg_ddg << indent << target_file << filename << target_line << line << "\n";

                if (!real_file.compare(filename) && real_line == line) {
                  targetIns.insert( & I);
                  debug << "real: " << filename << line << "\n";
                }

            }
                  

            if (!is_target) {
              for (auto & target: targets) {
                std::size_t found = target.find_last_of("/\\");
                if (found != std::string::npos)
                  target = target.substr(found + 1);

                std::size_t pos = target.find_last_of(":");
                std::string target_file = target.substr(0, pos);
                unsigned int target_line = atoi(target.substr(pos + 1).c_str());

                //cfg_ddg << indent << target_file << filename << target_line << line << "\n";

                if (!target_file.compare(filename) && target_line == line) {
                  is_target = true;
                }

              }
            }
          }
        }
      }
      */
      //debug << "done find targets" << "\n";
      //fprintf(stderr, "done find targets");
      
      //record
      /*
      for (auto node: brIns) {
        //cfg_ddg << indent << "br: " << node << "\n";
      }
      for (auto node: targetIns) {
        //cfg_ddg << indent << "target: " << node << "\n";
      }
      */

      //try to get the blocks that have data flow relationships with the target Ins and br Ins
      //find the Br ins that flows to targets
      std::queue < Value * > ctrl_que;
      std::set < Value * > hasAnalyzed;
      std::set < Value * > rel_bb_ins;
      std::string srcFilename, dstFilename;
      unsigned srcLine, dstLine;
      std::set < Function * > crtFuncs;

      for (auto ins: targetIns) {
        ctrl_que.push(ins);
        tar_ins.insert(ins);
      }
      //find all br ins along the path, we have to handle recursion here.
      
      while (!ctrl_que.empty()) {
        Value * crtVal = ctrl_que.front();
        ctrl_que.pop();
        if (! isa < Instruction > (crtVal)) {      
          continue;
        }
        Instruction * crtIns = dyn_cast < Instruction > (crtVal);
        //the ins has been analyzed
        if (hasAnalyzed.count(crtIns) == 1) {
          continue;
        }

        getDebugLoc( crtIns, srcFilename, srcLine);
        //fprintf(stderr, "control %s:%d\n", srcFilename.c_str(), srcLine);
        //it is a c or c++ file
        if(srcFilename.find(".h") == std::string::npos) {
          crtFuncs.insert(crtIns->getParent()->getParent());
          //fprintf(stderr, "add function %s\n", crtIns->getParent()->getParent()->getName().str().c_str());
        }
        hasAnalyzed.insert(crtIns);

        //insert the call sites
        if (funcToCall.count(crtIns->getParent()->getParent()) != 0){
          for (auto ins: funcToCall[crtIns->getParent()->getParent()]) {
            ctrl_que.push(ins);
          }
        }

        for (auto it = ctrl_rel.equal_range(crtIns); it.first != it.second; ++it.first) {

          Instruction * IT = dyn_cast < Instruction > (it.first -> second);
          getDebugLoc( IT, dstFilename, dstLine);

          Function* f1 = crtIns->getParent()->getParent();
          //get desins
          Value* desVal = it.first -> second;
          if (! isa < Instruction > (desVal)) {      
            continue;
          }
          Instruction* desIns = dyn_cast < Instruction > (desVal);
          Function* f2 = desIns->getParent()->getParent();

          //wants to introduce new function?
          if (crtFuncs.count(f2)==0) {
            //only .h files
            if (dstFilename.find(".h") != std::string::npos) {
              ctrl_que.push(it.first -> second);
              fprintf(stderr, "%s:%d to %s:%d\n", srcFilename.c_str(), srcLine, dstFilename.c_str(), dstLine);
            }
          }
          //intra-procederual data flow transfer
          else{
            //push its parent instruction
            ctrl_que.push(it.first -> second);
            fprintf(stderr, "%s:%d to %s:%d\n", srcFilename.c_str(), srcLine, dstFilename.c_str(), dstLine);
          }
        }
        //it is a function start instruction
        if (brIns.find(crtIns) != brIns.end()) {
          rel_bb_ins.insert(crtIns);
        }

      }
      
      std::set < Value *> total_ins = hasAnalyzed;


      /*
      for(auto ins_pair: ctrl_save) {
        std::string bb_name("");
        std::string filename;
        unsigned line;
        Value* first = ins_pair.first;
        if (isa < Instruction > (first)) {
          Instruction* first_ins = dyn_cast < Instruction > (first);
          getDebugLoc( first_ins, filename, line);
        }
        unsigned line1;
        for(auto val: ins_pair.second) {
          std::string bb_name1("");
          std::string filename1;
          if (isa < Instruction > (val)) {
            Instruction* val_ins = dyn_cast < Instruction > (val);
            getDebugLoc( val_ins, filename1, line1);
          }
          //debug << "ctrl_save: " << first << " " << getvaluestaticname(first) << filename <<  ":" << line << "->" << val << " " << getvaluestaticname(val) << filename1 << ":" << line1 << "\n";
        }
      }
      */


      //perform data flow analysis
      std::queue < Value * > data_que;
      for (auto tar: tar_ins) {
        data_que = std::queue < Value * > ();
        hasAnalyzed.clear();
        data_que.push(tar);
        while (!data_que.empty()) {
          Value * crtIns = data_que.front();
          data_que.pop();
          rel_ins.insert(crtIns);
          //the ins has been analyzed
          if (hasAnalyzed.count(crtIns) == 1) {
            continue;
          }
          hasAnalyzed.insert(crtIns);
          for (auto it = data_rel.equal_range(crtIns); it.first != it.second; ++it.first) {
            //push its parent instruction
            //cfg_ddg << indent << "find: " << it.first -> first << " " << it.first -> second << "\n";
            data_que.push(it.first -> second);
          }
        }
      }

      
      for(auto ins: rel_bb_ins) {
        rel_ins.insert(ins);
      }
      
      std::set< Value * > tmp = rel_ins;

      /*
      for(auto ins: tmp) {
        if (total_ins.count(ins)==0) {
          rel_ins.erase(ins);
        }
      }
      */
      //debug << "done resolve data flow" << "\n";
      fprintf(stderr, "done resolve data flow");

      
      //find pro of each ins
      hasAnalyzed.clear();

      count=0;
      for(auto ins: rel_ins) {
        count++;
        fprintf(stderr,  "@@@ %d, %d", count, rel_ins.size());
        std::set <Value *> em;
        find_pro(ins, targetIns, em);
        //pro_map[ins] = pro;
      }

      //debug << "done resolve distance" << "\n";
      fprintf(stderr, "done resolve distance");

      //rel_ins.insert(indirect.begin(), indirect.end());
      
      std::map < Value *,  float> pro_bb_map;
      //get basic block 

      int reach_ins_num = 0;

      for (auto bb: all_bb) {
        //only analyze the first instruction of the BB.
        BasicBlock * BT = dyn_cast < BasicBlock > (bb);
        for (auto II = BT -> begin(), IE = BT -> end(); II != IE; ++II) {
            Instruction * IT = dyn_cast < Instruction > (II);;
            std::set <Value *> em;
            find_pro(IT, targetIns, em);
            //fprintf(stderr,  "*** %d, %d", count, rel_ins.size());
            if (pro_map.count(IT)==1 && pro_map[IT]!=0) {
              reach_ins_num++;
            }
            break;
        }
      }

      for (auto rel: rel_ins) {
        //it is an instruction
        if (isa < Instruction > (rel)) {
          Instruction * ins = dyn_cast < Instruction > (rel);
          //cfg_ddg << indent << "rel blocks: " << rel << " " << ins -> getParent() << "\n";
          BasicBlock * bb = ins -> getParent();
          rel_bb.insert(bb);
          //update the BB distance
          if (pro_map.count(rel)==1) {
            if (pro_map[rel]==0) {
              continue;
            }
            float d = 1/pro_map[rel];
            if(pro_bb_map.count(bb)==0 || (pro_bb_map.count(bb)==1&&d<pro_bb_map[bb]&&(ins->getOpcode()!=Instruction::Call)))
              pro_bb_map[bb] = d;
          }
          else{
            pro_bb_map[bb] = -1;
          }
        }
      }

      //get bro of rel ins
      
      for(auto bb_pair: switch_map) {
        BasicBlock* first = bb_pair.first;
        std::set<BasicBlock *> cases = bb_pair.second;
        //at least one case is related
        if(pro_bb_map.count(first)) {
          for(auto c: cases){
            if(pro_bb_map.count(c)==0 || pro_bb_map[c]>pro_bb_map[first]) pro_bb_map[c]=pro_bb_map[first];
            rel_bb.insert(c);
          }
        }
      }
      
      std::vector<float> dis;
      for (auto pro: pro_bb_map) {
        if(std::find(dis.begin(), dis.end(), pro.second) == dis.end())
          dis.push_back(pro.second);
      }

      std::sort(dis.begin(), dis.end());

      std::set<std::string> saveLoc;
      //write basic blocks
      static const std::string Xlibs("/usr/");
      for (auto bb: rel_bb) {
        //cfg_ddg << "BB: " << bb << "\n";
        //get its location
        if (map_bb.count(bb) == 1){
          //we calculate BB loc once and we do not include *h files
          if(saveLoc.count(map_bb.at(bb))!=0 || map_bb.at(bb).find(".h")!=std::string::npos){
            continue;
          }
          saveLoc.insert(map_bb.at(bb));
          //we do not include "*.h"
          cfg_ddg << map_bb.at(bb) << "\n";
          if(pro_bb_map.count(bb)==1){
            if(pro_bb_map.at(bb)<2 && pro_bb_map.at(bb)>0 && pro_bb_map.at(bb)!=1.0) {
              pro_bb_map[bb] = 2;
            }
            if (pro_bb_map.at(bb)>256) {
              //pro_bb_map[bb] = 999999;
              int i;
              for (i=0; i<dis.size(); i++) {
                if (dis[i]==pro_bb_map.at(bb)) {
                  pro_bb_map[bb] = 256+i;
                  break;
                }
              }
            }
            distance << map_bb.at(bb) << "," << pro_bb_map[bb] << "\n";
          }
        }
      }

      debug << "total BB: " << total_BB << " reachable BB: " << reach_ins_num << "\n";

      cfg_ddg.close();

      return true;
    }
 };
}
char DFUZZPASS::ID = 0;
static RegisterPass < DFUZZPASS > X("DFUZZPASS", "DFUZZ Pass", false, false);

