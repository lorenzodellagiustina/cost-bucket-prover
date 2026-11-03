// CallAnalysis: the calls the top function may execute whose cost the counter cannot count.

#ifndef HUX_CALL_ANALYSIS_H
#define HUX_CALL_ANALYSIS_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include <map>
#include <string>

namespace hux
{

  struct CallSummary
  {
    // Calls to functions only declared in the module: callee name -> number of call sites. Their
    // cost is not counted (only the call instruction's).
    std::map<std::string, unsigned> External;
    // Inline assembly: not counted.
    unsigned InlineAsm = 0;
    // Calls through a function pointer: their callee is counted when it is defined in the module,
    // not when it is an external function.
    unsigned Indirect = 0;
  };

  // The calls `Top` may execute: those of the defined functions reachable from it through direct
  // calls and, once one of them makes an indirect call, of every function whose address is taken (a
  // conservative set of the pointer's possible callees).
  CallSummary analyzeCalls(llvm::Module &M, llvm::Function &Top);

} // namespace hux

#endif // HUX_CALL_ANALYSIS_H
