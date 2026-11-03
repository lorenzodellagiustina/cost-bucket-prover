#include "CostCounterPass.h"

#include "CallAnalysis.h"
#include "Report.h"
#include "Weights.h"

#include "llvm/IR/IRBuilder.h"

#include <utility>
#include <vector>

using namespace llvm;

namespace hux
{

  namespace
  {

    // The counter, created (initialized to 0) if the module doesn't declare it yet.
    GlobalVariable *getOrCreateCounter(Module &M)
    {
      Type *I64 = Type::getInt64Ty(M.getContext());
      if (GlobalVariable *G = M.getGlobalVariable(CounterName))
      {
        if (G->getValueType() != I64)
          report_fatal_error(Twine(PassName) + ": '" + CounterName + "' exists with another type");
        if (G->isDeclaration())
          G->setInitializer(ConstantInt::get(I64, 0));
        return G;
      }
      return new GlobalVariable(M, I64, /*isConstant=*/false, GlobalValue::ExternalLinkage,
                                ConstantInt::get(I64, 0), CounterName);
    }

    // The report's entry of each defined function, with its blocks' weights.
    std::vector<FunctionEntry>
    functionEntries(Module &M, const std::vector<std::pair<BasicBlock *, uint64_t>> &Blocks)
    {
      std::vector<FunctionEntry> Functions;
      for (Function &F : M)
      {
        if (F.isDeclaration())
          continue;
        FunctionEntry Entry;
        Entry.Name = F.getName().str();
        for (auto &[BB, Weight] : Blocks)
        {
          if (BB->getParent() != &F)
            continue;
          BlockEntry Block;
          Block.Index = Entry.Blocks.size();
          Block.Name = BB->hasName() ? BB->getName().str() : "";
          Block.Weight = Weight;
          Entry.Blocks.push_back(std::move(Block));
        }
        Functions.push_back(std::move(Entry));
      }
      return Functions;
    }

  } // namespace

  PreservedAnalyses CostCounterPass::run(Module &M, ModuleAnalysisManager &)
  {
    auto W = loadWeights(Opts.Weights);
    if (!W)
      report_fatal_error(W.takeError());

    Function *Top = M.getFunction(Opts.Top);
    if (!Top || Top->isDeclaration())
      report_fatal_error(Twine(PassName) + ": top function '" + Opts.Top +
                         "' is not defined in the module");

    // Weights are computed on the original instructions, before any insertion.
    std::vector<std::pair<BasicBlock *, uint64_t>> Blocks;
    for (Function &F : M)
      for (BasicBlock &BB : F)
      {
        uint64_t Sum = 0;
        for (Instruction &I : BB)
          Sum += W->of(I);
        Blocks.emplace_back(&BB, Sum);
      }

    GlobalVariable *Counter = getOrCreateCounter(M);
    Type *I64 = Type::getInt64Ty(M.getContext());
    for (auto &[BB, Weight] : Blocks)
    {
      if (Weight == 0)
        continue;
      IRBuilder<> B(&*BB->getFirstInsertionPt());
      Value *Old = B.CreateLoad(I64, Counter, "hux.cost");
      B.CreateStore(B.CreateAdd(Old, ConstantInt::get(I64, Weight)), Counter);
    }

    if (!Opts.Report.empty())
    {
      Report R;
      R.Top = Opts.Top;
      R.Counter = CounterName;
      R.Functions = functionEntries(M, Blocks);
      R.Calls = analyzeCalls(M, *Top);
      if (Error E = writeReport(Opts.Report, R))
        report_fatal_error(std::move(E));
    }
    return PreservedAnalyses::none();
  }

} // namespace hux
