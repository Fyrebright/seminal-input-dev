#ifndef __412PROJ_SEM_VAL_H
#define __412PROJ_SEM_VAL_H

#include "FindIOVals.h"
#include "utils.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

#include <set>

// Forward declarations
namespace llvm {
class Instruction;
class Value;
class Function;
class Module;
class raw_ostream;

} // namespace llvm

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
class SeminalInput : public llvm::AnalysisInfoMixin<SeminalInput> {
public:
  struct SeminalInputInfo {
  public:
    // Can reuse IOValMetadata struct
    std::vector<IOValMetadata> semValsMetadata;

    bool invalidate(llvm::Function &F,
                    const llvm::PreservedAnalyses &PA,
                    llvm::FunctionAnalysisManager::Invalidator &Inv) {
      return false;
    }
  };

  using Result = SeminalInputInfo;
  // This is one of the standard run() member functions expected by
  // PassInfoMixin. When the pass is executed by the new PM, this is the
  // function that will be called.
  Result run(llvm::Function &Func, llvm::FunctionAnalysisManager &FAM);
  // This is a helper run() member function overload which can be called by the
  // legacy pass (or any other code) without having to supply a
  // FunctionAnalysisManager argument.
  Result run(llvm::Function &Func);

private:
  friend struct llvm::AnalysisInfoMixin<SeminalInput>;
  static llvm::AnalysisKey Key;
};

//------------------------------------------------------------------------------
// New PM interface for the printer pass
//------------------------------------------------------------------------------
class SeminalInputPrinter : public llvm::PassInfoMixin<SeminalInputPrinter> {
public:
  explicit SeminalInputPrinter(llvm::raw_ostream &OutStream) : OS(OutStream) {};

  llvm::PreservedAnalyses run(llvm::Function &Func,
                              llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};

//------------------------------------------------------------------------------
// New PM interface for the graphing pass
//------------------------------------------------------------------------------
class SeminalInputGraph : public llvm::PassInfoMixin<SeminalInputPrinter> {
public:
  explicit SeminalInputGraph(llvm::raw_ostream &OutStream) : OS(OutStream) {};

  llvm::PreservedAnalyses run(llvm::Function &Func,
                              llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};

#endif // __412PROJ_SEM_VAL_H