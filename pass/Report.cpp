#include "Report.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

LLVM_YAML_IS_SEQUENCE_VECTOR(hux::BlockEntry)
LLVM_YAML_IS_SEQUENCE_VECTOR(hux::FunctionEntry)
LLVM_YAML_IS_STRING_MAP(unsigned)

namespace llvm
{
  namespace yaml
  {

    template <>
    struct MappingTraits<hux::BlockEntry>
    {
      static void mapping(IO &Io, hux::BlockEntry &B)
      {
        Io.mapRequired("index", B.Index);
        Io.mapRequired("name", B.Name);
        Io.mapRequired("weight", B.Weight);
      }
    };

    template <>
    struct MappingTraits<hux::FunctionEntry>
    {
      static void mapping(IO &Io, hux::FunctionEntry &F)
      {
        Io.mapRequired("name", F.Name);
        Io.mapRequired("blocks", F.Blocks);
      }
    };

    // The uncounted calls only: the indirect ones are a separate key of the report.
    template <>
    struct MappingTraits<hux::CallSummary>
    {
      static void mapping(IO &Io, hux::CallSummary &C)
      {
        Io.mapRequired("external", C.External);
        Io.mapRequired("inline_asm", C.InlineAsm);
      }
    };

    template <>
    struct MappingTraits<hux::Report>
    {
      static void mapping(IO &Io, hux::Report &R)
      {
        Io.mapRequired("top", R.Top);
        Io.mapRequired("counter", R.Counter);
        Io.mapRequired("functions", R.Functions);
        Io.mapRequired("uncounted_calls", R.Calls);
        Io.mapRequired("indirect_calls", R.Calls.Indirect);
      }
    };

  } // namespace yaml
} // namespace llvm

namespace hux
{

  Error writeReport(StringRef Path, Report &R)
  {
    std::error_code EC;
    raw_fd_ostream OS(Path, EC, sys::fs::OF_Text);
    if (EC)
      return createStringError(EC, "cannot write report '%s'", Path.str().c_str());
    yaml::Output Out(OS);
    Out << R;
    return Error::success();
  }

} // namespace hux
