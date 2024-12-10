#ifndef FIND_IO_VALS_H
#define FIND_IO_VALS_H

#include <set>
#include <vector>
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "utils.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/FormatVariadic.h"

// Forward declarations
namespace llvm {
class Instruction;
class Value;
class Function;
class Module;
class raw_ostream;

} // namespace llvm

const std::set<llvm::StringRef> INPUT_FUNCTIONS{
    "fdopen",
    "fgetc",
    "fgetpos",
    "fgets",
    "fopen",
    "fread",
    "freopen",
    "fscanf",
    "getc_unlocked",
    "getc",
    "getchar_unlocked",
    "getchar",
    "getenv",
    "getopt",
    "gets",
    "getw",
    "popen",
    "scanf",
    "sscanf",
    "ungetc",
};

const std::set<llvm::StringRef> FILE_DEP_FUNCTIONS{
    "fgetc",
    "fgetpos",
    "fgets",
    "fread",
    "fscanf",
};

inline bool isInputFunc(llvm::Function &F, llvm::StringRef &name) {
  name = F.getName();

  // Remove prefix if present
  name.consume_front("__isoc99_");
  // LLVM_DEBUG(llvm::dbgs() << "after consume " << name << "\n");

  return INPUT_FUNCTIONS.find(name) != INPUT_FUNCTIONS.cend();
}

// Struct to map input values to the instruction that assigns them
struct IOValMetadata {
  llvm::Value *val;
  llvm::Instruction *inst;
  std::string funcName;
  bool isFile = false;
  std::string comment = "";
  int lineNum = -1;
};

inline void
_pretty_print_meta(const std::vector<IOValMetadata> &ioValsMetadata) {
  for(const auto &IOVal: ioValsMetadata) {
    llvm::dbgs() << formatv(
        "[{0},{1},{2}]\n", IOVal.val->getName(), IOVal.funcName, IOVal.lineNum);
  }
}

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
/**
 * @brief Analysis pass to find IO-dependent values in a function.
 */
class FindIOVals : public llvm::AnalysisInfoMixin<FindIOVals> {
public:

  struct IOValsInfo {
  public:
    std::set<llvm::Value *> ioVals;
    std::vector<IOValMetadata> ioValsMetadata;


    bool invalidate(llvm::Function &F,
                    const llvm::PreservedAnalyses &PA,
                    llvm::FunctionAnalysisManager::Invalidator &Inv) {
      return false;
    }
  };

  using Result = IOValsInfo;

  Result run(llvm::Function &Func, llvm::FunctionAnalysisManager &FAM);

  Result run(llvm::Function &Func);

private:
  friend struct llvm::AnalysisInfoMixin<FindIOVals>;
  static llvm::AnalysisKey Key;
};

struct FuncReturnIO : public llvm::AnalysisInfoMixin<FuncReturnIO> {

  class Result {
  public:
    bool returnIsIO;
  };
};

//------------------------------------------------------------------------------
// New PM interface for the printer pass
//------------------------------------------------------------------------------
class FindIOValsPrinter : public llvm::PassInfoMixin<FindIOValsPrinter> {
public:
  explicit FindIOValsPrinter(llvm::raw_ostream &OutStream) : OS(OutStream) {};

  llvm::PreservedAnalyses run(llvm::Function &Func,
                              llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};

#endif // FIND_IO_VALS_H