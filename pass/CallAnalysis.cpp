#include "CallAnalysis.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"

using namespace llvm;

namespace hux
{

  CallSummary analyzeCalls(Module &M, Function &Top)
  {
    CallSummary Summary;
    SetVector<Function *> Reachable;
    Reachable.insert(&Top);
    for (size_t K = 0; K < Reachable.size(); ++K)
    {
      for (Instruction &I : instructions(*Reachable[K]))
      {
        auto *CB = dyn_cast<CallBase>(&I);
        if (!CB || isa<IntrinsicInst>(CB))
          continue;
        if (CB->isInlineAsm())
        {
          ++Summary.InlineAsm;
          continue;
        }
        Function *Callee = CB->getCalledFunction();
        if (!Callee)
        {
          if (Summary.Indirect++ == 0)
            for (Function &F : M)
              if (!F.isDeclaration() && F.hasAddressTaken())
                Reachable.insert(&F);
        }
        else if (Callee->isDeclaration())
        {
          ++Summary.External[Callee->getName().str()];
        }
        else
        {
          Reachable.insert(Callee);
        }
      }
    }
    return Summary;
  }

} // namespace hux
