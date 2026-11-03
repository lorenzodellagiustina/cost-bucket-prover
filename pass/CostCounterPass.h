// CostCounterPass: makes the execution cost of a function an ordinary program variable.
//
// In every function defined in the module, each basic block gets `__hux_cost += w` at its first
// insertion point, where w is the sum of the weights of the block's original instructions.
//
// Note: the counter accumulates over everything executed: measuring a single call (the top
// function's) is up to the caller, which resets it right before (HUX_START() in include/hux.h).
//
// Usage (new pass manager, see Plugin.cpp):
//   opt -load-pass-plugin=libHuxCostCounter.so \
//       -passes='hux-cost-counter<top=FN;weights=W.yaml;report=R.yaml>' -S in.ll -o out.ll
//
// Parameters:
//   top     (required): the function whose calls are analyzed for the report.
//   weights (optional, input): a YAML file with the cost of each instruction (see Weights.h).
//   report  (optional, output): a YAML file the pass writes, listing the blocks it instrumented
//     with their weights, and the calls the top function may execute whose cost is not counted.

#ifndef HUX_COST_COUNTER_PASS_H
#define HUX_COST_COUNTER_PASS_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

#include <string>

namespace hux
{

  // `constexpr` needs `inline` to avoid multiple definitions in different translation units.
  inline constexpr const char *PassName = "hux-cost-counter";
  inline constexpr const char *CounterName = "__hux_cost";

  struct Options
  {
    std::string Top;
    std::string Weights; // Path of the weights file; empty: every instruction weighs 1.
    std::string Report;  // Path of the report; empty: no report.
  };

  class CostCounterPass : public llvm::PassInfoMixin<CostCounterPass>
  {
  public:
    explicit CostCounterPass(Options O) : Opts(std::move(O)) {}

    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);

  private:
    Options Opts;
  };

} // namespace hux

#endif // HUX_COST_COUNTER_PASS_H
