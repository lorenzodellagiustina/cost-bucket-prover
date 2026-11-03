// CostCounterPass: makes the execution cost of a function an ordinary program variable.
//
// In every function defined in the module, each basic block gets `__hux_cost += w` at its first
// insertion point, where w is the sum of the weights of the block's original instructions.
//
// Note: the counter accumulates over everything executed: measuring a single call (the top
// function's) is up to the caller, which resets it right before (HUX_START() in include/hux.h).
//
// Usage (new pass manager):
//   opt -load-pass-plugin=libHuxCostCounter.so \
//       -passes='hux-cost-counter<top=FN;weights=W.json;report=R.json>' in.bc -o out.bc
//
// weights (optional): {"default": 1, "opcodes": {"load": 2, "call": 4}}; keys are LLVM opcode
//   names. Without a file, every instruction weighs 1. Debug intrinsics always weigh 0.
// report (optional): JSON listing the instrumented blocks with their weights, plus the calls that
//   the top function may execute whose callee has no code in the module (external declarations,
//   inline asm). The cost of those callees is NOT counted; the report makes that visible. It also
//   counts the indirect calls, whose callees are counted if they are defined in the module.

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <string>

using namespace llvm;

namespace
{

  constexpr const char *CounterName = "__hux_cost";
  constexpr const char *PassName = "hux-cost-counter";

  struct Options
  {
    std::string Top;
    std::string Weights;
    std::string Report;
  };

  struct Weights
  {
    uint64_t Default = 1;
    StringMap<uint64_t> ByOpcode;

    uint64_t of(const Instruction &I) const
    {
      if (isa<DbgInfoIntrinsic>(I))
        return 0;
      auto It = ByOpcode.find(I.getOpcodeName());
      return It == ByOpcode.end() ? Default : It->second;
    }
  };

  Expected<Weights> loadWeights(StringRef Path)
  {
    Weights W;
    if (Path.empty())
      return W;
    auto Buf = MemoryBuffer::getFile(Path);
    if (!Buf)
      return createStringError(Buf.getError(), "cannot read weights file '%s'",
                               Path.str().c_str());
    auto Root = json::parse((*Buf)->getBuffer());
    if (!Root)
      return Root.takeError();
    const json::Object *Obj = Root->getAsObject();
    if (!Obj)
      return createStringError(inconvertibleErrorCode(), "weights: top level must be an object");
    if (auto D = Obj->getInteger("default"))
    {
      if (*D < 0)
        return createStringError(inconvertibleErrorCode(), "weights: negative default");
      W.Default = *D;
    }
    if (const json::Object *Ops = Obj->getObject("opcodes"))
    {
      for (const auto &KV : *Ops)
      {
        auto V = KV.second.getAsInteger();
        if (!V || *V < 0)
          return createStringError(inconvertibleErrorCode(),
                                   "weights: opcode '%s' needs a non-negative integer",
                                   KV.first.str().c_str());
        W.ByOpcode[KV.first.str()] = *V;
      }
    }
    return W;
  }

  // Parses "top=FN;weights=PATH;report=PATH" (the text between the angle brackets).
  Expected<Options> parseOptions(StringRef Params)
  {
    Options O;
    while (!Params.empty())
    {
      StringRef Item;
      std::tie(Item, Params) = Params.split(';');
      StringRef Key, Value;
      std::tie(Key, Value) = Item.split('=');
      if (Key == "top")
        O.Top = Value.str();
      else if (Key == "weights")
        O.Weights = Value.str();
      else if (Key == "report")
        O.Report = Value.str();
      else
        return createStringError(inconvertibleErrorCode(), "%s: unknown parameter '%s'", PassName,
                                 Key.str().c_str());
    }
    if (O.Top.empty())
      return createStringError(inconvertibleErrorCode(), "%s: missing parameter 'top'", PassName);
    return O;
  }

  class CostCounterPass : public PassInfoMixin<CostCounterPass>
  {
  public:
    explicit CostCounterPass(Options O) : Opts(std::move(O)) {}

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &)
    {
      auto W = loadWeights(Opts.Weights);
      if (!W)
        report_fatal_error(W.takeError());

      Function *Top = M.getFunction(Opts.Top);
      if (!Top || Top->isDeclaration())
        report_fatal_error(Twine(PassName) + ": top function '" + Opts.Top +
                           "' is not defined in the module");

      // The calls the top function may execute, for the report: from the defined functions reachable
      // from Top via direct calls and, once an indirect call is reachable, via any function whose
      // address is taken (a conservative set of its possible callees).
      SetVector<Function *> Reachable;
      std::map<std::string, unsigned> External; // callee name -> number of call sites
      unsigned Indirect = 0, InlineAsm = 0;
      Reachable.insert(Top);
      for (size_t K = 0; K < Reachable.size(); ++K)
      {
        for (Instruction &I : instructions(*Reachable[K]))
        {
          auto *CB = dyn_cast<CallBase>(&I);
          if (!CB || isa<IntrinsicInst>(CB))
            continue;
          if (CB->isInlineAsm())
          {
            ++InlineAsm;
            continue;
          }
          Function *Callee = CB->getCalledFunction();
          if (!Callee)
          {
            if (Indirect++ == 0)
              for (Function &F : M)
                if (!F.isDeclaration() && F.hasAddressTaken())
                  Reachable.insert(&F);
          }
          else if (Callee->isDeclaration())
          {
            ++External[Callee->getName().str()];
          }
          else
          {
            Reachable.insert(Callee);
          }
        }
      }

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
        writeReport(M, Blocks, External, Indirect, InlineAsm);
      return PreservedAnalyses::none();
    }

  private:
    Options Opts;

    static GlobalVariable *getOrCreateCounter(Module &M)
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

    void writeReport(Module &M, const std::vector<std::pair<BasicBlock *, uint64_t>> &Blocks,
                     const std::map<std::string, unsigned> &External, unsigned Indirect,
                     unsigned InlineAsm) const
    {
      json::Array Funcs;
      for (Function &F : M)
      {
        if (F.isDeclaration())
          continue;
        json::Array BBs;
        unsigned Index = 0;
        for (auto &[BB, Weight] : Blocks)
        {
          if (BB->getParent() != &F)
            continue;
          BBs.push_back(json::Object{{"index", Index++},
                                     {"name", BB->hasName() ? BB->getName().str() : ""},
                                     {"weight", (int64_t)Weight}});
        }
        Funcs.push_back(json::Object{{"name", F.getName().str()}, {"blocks", std::move(BBs)}});
      }
      json::Object Ext;
      for (auto &[Name, N] : External)
        Ext[Name] = N;
      json::Object Root{
          {"top", Opts.Top},
          {"counter", CounterName},
          {"functions", std::move(Funcs)},
          {"uncounted_calls", json::Object{{"external", std::move(Ext)}, {"inline_asm", InlineAsm}}},
          {"indirect_calls", Indirect}};
      std::error_code EC;
      raw_fd_ostream OS(Opts.Report, EC, sys::fs::OF_Text);
      if (EC)
        report_fatal_error(Twine(PassName) + ": cannot write report '" + Opts.Report +
                           "': " + EC.message());
      OS << formatv("{0:2}", json::Value(std::move(Root))) << "\n";
    }
  };

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo()
{
  return {LLVM_PLUGIN_API_VERSION, PassName, "0.1", [](PassBuilder &PB)
          {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>)
                {
                  if (!Name.consume_front(PassName))
                    return false;
                  if (!Name.consume_front("<") || !Name.consume_back(">"))
                    report_fatal_error(Twine(PassName) + ": expected " + PassName +
                                       "<top=FN;...>");
                  auto O = parseOptions(Name);
                  if (!O)
                    report_fatal_error(O.takeError());
                  MPM.addPass(CostCounterPass(std::move(*O)));
                  return true;
                });
          }};
}
