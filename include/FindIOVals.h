#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include "llvm/IR/InstVisitor.h"
#include <set>
#include <vector>

// Forward declarations
namespace llvm {

class Value;
class Function;
class Module;
class raw_ostream;

} // namespace llvm

const std::set<llvm::StringRef> INPUT_FUNCTIONS{
    // "fdopen",
    "fgetc",
    // "fgetpos",
    "fgets",
    "fopen",
    "fread",
    // "freopen",
    "fscanf",
    "getc",
    "getc_unlocked",
    "getchar",
    "getchar_unlocked",
    "getopt",
    "gets",
    "getw",
    // "popen",
    "scanf",
    "sscanf",
    "ungetc",
};

inline bool isInputFunc(llvm::Function &F, llvm::StringRef &name) {
  name = F.getName();

  // Remove prefix if present
  name.consume_front("__isoc99_");
  // errs() << "after consume " << name << "\n";

  return INPUT_FUNCTIONS.find(name) != INPUT_FUNCTIONS.cend();
}

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
class FindIOVals : public llvm::AnalysisInfoMixin<FindIOVals> {
  struct IOValsInfo {
    public:
    std::vector<llvm::Value *> ioVals;

      bool invalidate(llvm::Function &F, const llvm::PreservedAnalyses &PA,
                  llvm::FunctionAnalysisManager::Invalidator &Inv) {
                    return false;
                  }
  };

public:
  using Result = IOValsInfo;
  // This is one of the standard run() member functions expected by
  // PassInfoMixin. When the pass is executed by the new PM, this is the
  // function that will be called.
  Result run(llvm::Function &Func, llvm::FunctionAnalysisManager &FAM);
  // This is a helper run() member function overload which can be called by the
  // legacy pass (or any other code) without having to supply a
  // FunctionAnalysisManager argument.
  Result run(llvm::Function &Func);

private:
  friend struct llvm::AnalysisInfoMixin<FindIOVals>;
  static llvm::AnalysisKey Key;
};

//------------------------------------------------------------------------------
// New PM interface for the printer pass
//------------------------------------------------------------------------------
class FindIOValsPrinter : public llvm::PassInfoMixin<FindIOValsPrinter> {
public:
  explicit FindIOValsPrinter(llvm::raw_ostream &OutStream) : OS(OutStream){};

  llvm::PreservedAnalyses run(llvm::Function &Func,
                              llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};