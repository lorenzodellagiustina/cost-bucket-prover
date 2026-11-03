// Weights: the cost of each LLVM instruction, read from a YAML file.

#ifndef HUX_WEIGHTS_H
#define HUX_WEIGHTS_H

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Error.h"

#include <cstdint>

namespace hux
{

  // The cost of each instruction, by its LLVM opcode name (e.g. `load`, `call`); `Default` for the
  // opcodes not listed. Debug intrinsics always weigh 0.
  struct Weights
  {
    uint64_t Default = 1;
    llvm::StringMap<uint64_t> ByOpcode;

    uint64_t of(const llvm::Instruction &I) const;
  };

  // Reads the weights file at `Path`, e.g.:
  //   default: 1
  //   opcodes:
  //     load: 2
  //     call: 4
  // An empty path means every instruction weighs 1. An unknown key, or a value that is not a
  // non-negative integer, is an error.
  llvm::Expected<Weights> loadWeights(llvm::StringRef Path);

} // namespace hux

#endif // HUX_WEIGHTS_H
