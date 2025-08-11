#include "TypeAnalysis.h"


void TypeAnalysis::addFunc(Function *Func){
     Ctx->AddressTakenFuncs.insert(Func);
}

void TypeAnalysis::printAddressTakenFunc(){
     
     std::string FileName( Ctx->DotFile + "/addressTakenFunc.txt");
     std::error_code error;
     raw_fd_ostream O(FileName, error);

     
     for (auto F: Ctx->AddressTakenFuncs){
         O << F->getName().str() << "\n";
     }

     O.close();
}


void TypeAnalysis::addTypeIndirectCallEdge(Function *srcNode, FuncSet &destNode){
     for (auto callee: destNode)
         Ctx->TypeIndirectCallMap[srcNode].insert(callee);
}

void TypeAnalysis::addIndirectCallSite(Function *srcFun, CallBase* CB){
     Ctx->IndirectCallSite[srcFun].insert(CB);
} 



bool TypeAnalysis::fuzzyTypeMatch(Type *Ty1, Type *Ty2){
  if (Ty1 == Ty2)
     return true;

  while (Ty1->isPointerTy() && Ty2->isPointerTy()){
        Ty1 = Ty1->getPointerElementType();
        Ty2 = Ty2->getPointerElementType();
  }

  if (Ty1->isStructTy() && Ty2->isStructTy() &&
                  (Ty1->getStructName().equals(Ty2->getStructName())))
     return true;
  if (Ty1->isIntegerTy() && Ty2->isIntegerTy() &&
                   Ty1->getIntegerBitWidth() == Ty2->getIntegerBitWidth())
     return true;

  // TODO: more types to be supported.

  // Make the type analysis conservative: assume general
  // pointers, e.g., "void *" and "char *", are equivalent
  // to any pointer type and integer type.

  if (
       ( Ty1 == Int8PtrTy && (Ty2->isPointerTy() || Ty2 == IntPtrTy))
        ||
       ( Ty2 == Int8PtrTy && (Ty1->isPointerTy() || Ty1 == IntPtrTy))
     )   

     return true;

  return false;


}


/*find targets of indirect calls based on function-type analysis:
  as long as the number and the type of parameters of a function
  matches with the ones of the callsite, we say the function is a
  possible target of this call( borrow from Crix static analyzer)
*/

void TypeAnalysis::findCalleesByType(CallBase *CB, FuncSet &FS){
  if (CB->isInlineAsm())
     return;

  for (Function *F: Ctx->AddressTakenFuncs) {

    if (F->getFunctionType()->isVarArg()){
      //Compare only known args in VarArg.
    }   

    // Otherwise, the numbers of args should be equal.
    else if (F->arg_size() != CB->arg_size()){
            continue;
    }

    if (F->isIntrinsic()) {
       continue;
    }

    // Type matching on args.
    bool Matched = true;
    User::op_iterator AI = CB->arg_begin();
    for (Function::arg_iterator FI = F->arg_begin(),
    FE = F->arg_end();
                FI != FE; ++FI, ++AI){
         //Check type mis-matches.
         //Get defined type on callee side.
         Type *DefinedTy = FI->getType();
         //Get actual type on caller side.
         Type *ActualTy = (*AI)->getType();

         if (!fuzzyTypeMatch(DefinedTy, ActualTy)){
            Matched = false;
            break;
         }
    }

    //If args are matched, further check return types
    if (Matched){
       Type *RTy1 = F->getReturnType();
       Type *RTy2 = CB->getType();
       if (!fuzzyTypeMatch(RTy1, RTy2))
          Matched = false;

    }

    if (Matched)
       FS.insert(F);

  }
}

