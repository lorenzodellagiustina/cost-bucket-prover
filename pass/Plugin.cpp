// Registers CostCounterPass with the new pass manager, as `hux-cost-counter<top=FN;...>`.

#include "CostCounterPass.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Error.h"

using namespace llvm;

namespace
{

  // Parses options inside the angle brackets.
  Expected<hux::Options> parseOptions(StringRef Params)
  {
    hux::Options O;
    // Parse one parameter at a time, separated by semicolons, as key=value.
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
        return createStringError(inconvertibleErrorCode(), "%s: unknown parameter '%s'",
                                 hux::PassName, Key.str().c_str());
    }
    if (O.Top.empty())
      return createStringError(inconvertibleErrorCode(), "%s: missing parameter 'top'",
                               hux::PassName);
    return O;
  }

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo()
{
  return {LLVM_PLUGIN_API_VERSION, hux::PassName, "0.1", [](PassBuilder &PB)
          {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>)
                {
                  if (!Name.consume_front(hux::PassName))
                    return false;
                  if (!Name.consume_front("<") || !Name.consume_back(">"))
                    report_fatal_error(Twine(hux::PassName) + ": expected " + hux::PassName +
                                       "<top=FN;...>");
                  auto O = parseOptions(Name);
                  if (!O)
                    report_fatal_error(O.takeError());
                  MPM.addPass(hux::CostCounterPass(std::move(*O)));
                  return true;
                });
          }};
}
