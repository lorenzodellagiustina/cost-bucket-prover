#include "Weights.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLTraits.h"

#include <map>
#include <string>

using namespace llvm;

namespace
{

  // The weights file, as read.
  struct WeightsFile
  {
    uint64_t Default = 1;
    std::map<std::string, uint64_t> Opcodes;
  };

} // namespace

LLVM_YAML_IS_STRING_MAP(uint64_t)

namespace llvm
{
  namespace yaml
  {

    template <>
    struct MappingTraits<WeightsFile>
    {
      static void mapping(IO &Io, WeightsFile &W)
      {
        Io.mapOptional("default", W.Default, uint64_t(1));
        Io.mapOptional("opcodes", W.Opcodes);
      }
    };

  } // namespace yaml
} // namespace llvm

namespace hux
{

  uint64_t Weights::of(const Instruction &I) const
  {
    if (isa<DbgInfoIntrinsic>(I))
      return 0;
    auto It = ByOpcode.find(I.getOpcodeName());
    return It == ByOpcode.end() ? Default : It->second;
  }

  Expected<Weights> loadWeights(StringRef Path)
  {
    Weights W;
    if (Path.empty())
      return W;
    auto Buf = MemoryBuffer::getFile(Path);
    if (!Buf)
      return createStringError(Buf.getError(), "cannot read weights file '%s'",
                               Path.str().c_str());
    // Unknown keys and values that are not non-negative integers are errors (the parser prints
    // where).
    WeightsFile File;
    yaml::Input In((*Buf)->getBuffer());
    In >> File;
    if (In.error())
      return createStringError(In.error(), "cannot parse weights file '%s'", Path.str().c_str());
    W.Default = File.Default;
    for (const auto &[Opcode, Weight] : File.Opcodes)
      W.ByOpcode[Opcode] = Weight;
    return W;
  }

} // namespace hux
