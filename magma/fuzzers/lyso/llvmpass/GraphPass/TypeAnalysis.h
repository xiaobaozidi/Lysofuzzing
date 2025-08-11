#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H


#include "GraphPass.h"
using namespace llvm;
using namespace std;


class TypeAnalysis {


  private:
    
     GlobalContext *Ctx;

     bool fuzzyTypeMatch(Type *Ty1, Type *Ty2);  
    

  public:
    
     const DataLayout *DL;
     //char* or void*
     Type *Int8PtrTy;
     //long interger type
     Type *IntPtrTy;

     TypeAnalysis(GlobalContext *Ctx_) : Ctx(Ctx_) {};

     /* Add address taken function */
     void addFunc(Function* Func);

     // Use type-based analysis to find all potential callee functions of indirect calls.
     void findCalleesByType(CallBase *CB, FuncSet &FS);
    
     /* Add the Indirect call edge from type analysis */
     void addTypeIndirectCallEdge(Function *srcNode, FuncSet &destNode);

      
     void addIndirectCallSite(Function *srcFun, CallBase *CB);


     void printAddressTakenFunc(); 
};

#endif

