#ifndef __412PROJ_FIND_IO_VALS_H
#define __412PROJ_FIND_IO_VALS_H

#include "utils.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

#include <set>
#include <vector>

// Forward declarations
namespace llvm {
class Instruction;
class Value;
class Function;
class Module;
class raw_ostream;

} // namespace llvm

const std::set<llvm::StringRef> INPUT_FUNCTIONS{// "fdopen",
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
                                                "getenv"};

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
};

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
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

struct FuncReturnIO : public llvm::AnalysisInfoMixin<FuncReturnIO> {
  /**
   * @brief Functions with retvals dependent on input
   */
  class Result {
  public:
    bool returnIsIO;

    // bool invalidate(llvm::Function &F, const llvm::PreservedAnalyses &PA,
    //               llvm::FunctionAnalysisManager::Invalidator &Inv);
  };
  Result run(llvm::Function &, llvm::FunctionAnalysisManager &);

private:
  // A special type used by analysis passes to provide an address that
  // identifies that particular analysis pass type.
  static llvm::AnalysisKey Key;
  friend struct llvm::AnalysisInfoMixin<FuncReturnIO>;
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

#endif // __412PROJ_FIND_IO_VALS_H