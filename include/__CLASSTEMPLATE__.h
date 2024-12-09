#ifndef __412PROJ___HEADERTEMPLATE___H
#define __412PROJ___HEADERTEMPLATE___H

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
class __CLASSTEMPLATE__ : public llvm::AnalysisInfoMixin<__CLASSTEMPLATE__> {
public:
  struct __TEMPLATE__Info {
  public:
    //// TODO: Replace fields
    std::set<llvm::Instruction *> __DATA_TEMPLATE__;

    bool invalidate(llvm::Function &F,
                    const llvm::PreservedAnalyses &PA,
                    llvm::FunctionAnalysisManager::Invalidator &Inv) {
      return false;
    }
  };

  using Result = __TEMPLATE__Info;
  // This is one of the standard run() member functions expected by
  // PassInfoMixin. When the pass is executed by the new PM, this is the
  // function that will be called.
  Result run(llvm::Function &Func, llvm::FunctionAnalysisManager &FAM);
  // This is a helper run() member function overload which can be called by the
  // legacy pass (or any other code) without having to supply a
  // FunctionAnalysisManager argument.
  Result run(llvm::Function &Func);

private:
  friend struct llvm::AnalysisInfoMixin<__CLASSTEMPLATE__>;
  static llvm::AnalysisKey Key;
};

//------------------------------------------------------------------------------
// New PM interface for the printer pass
//------------------------------------------------------------------------------
class __CLASSTEMPLATE__Printer
    : public llvm::PassInfoMixin<__CLASSTEMPLATE__Printer> {
public:
  explicit __CLASSTEMPLATE__Printer(llvm::raw_ostream &OutStream)
      : OS(OutStream) {};

  llvm::PreservedAnalyses run(llvm::Function &Func,
                              llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};

#endif // __412PROJ___HEADERTEMPLATE___H