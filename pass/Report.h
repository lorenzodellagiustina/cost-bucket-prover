// Report: what the pass instrumented, and the calls whose cost it cannot count, written as YAML.

#ifndef HUX_REPORT_H
#define HUX_REPORT_H

#include "CallAnalysis.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstdint>
#include <string>
#include <vector>

namespace hux
{

  struct BlockEntry
  {
    unsigned Index = 0;
    std::string Name;
    uint64_t Weight = 0;
  };

  struct FunctionEntry
  {
    std::string Name;
    std::vector<BlockEntry> Blocks;
  };

  struct Report
  {
    std::string Top;
    std::string Counter;
    std::vector<FunctionEntry> Functions;
    CallSummary Calls;
  };

  // Writes `R` to `Path` as YAML:
  //   top: FN
  //   counter: __hux_cost
  //   functions: [{name, blocks: [{index, name, weight}]}]
  //   uncounted_calls: {external: {callee: call sites}, inline_asm: N}
  //   indirect_calls: N
  llvm::Error writeReport(llvm::StringRef Path, Report &R);

} // namespace hux

#endif // HUX_REPORT_H
