/**
 * @file FindInputValues.h
 * @brief
 * @see \link https://github.com/banach-space/llvm-tutor \endlink
 */
#ifndef LLVM_TUTOR_RIV_H
#define LLVM_TUTOR_RIV_H

#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"

#include <set>

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

inline bool isLibFunc(llvm::Function &F, llvm::TargetLibraryInfo &TLI) {
  // errs() << "checking if " << F.getName() << " is a LibFunc\n";

  // if(EVIL_FUNCTIONS.find(F.getName()) != EVIL_FUNCTIONS.cend()) {
  //   return true;
  // }

  llvm::LibFunc _;
  return TLI.getLibFunc(F.getName(), _);
}

inline bool isInputFunc(llvm::Function &F, llvm::StringRef &name) {
  name = F.getName();

  // Remove prefix if present
  name.consume_front("__isoc99_");
  // errs() << "after consume " << name << "\n";

  return INPUT_FUNCTIONS.find(name) != INPUT_FUNCTIONS.cend();
}

struct FindInputValues : public llvm::AnalysisInfoMixin<FindInputValues> {
  /**
   * @brief Map each function to values dependent on input
   */
  using Result = std::vector<llvm::Value *>;
  Result run(llvm::Module &, llvm::ModuleAnalysisManager &);

private:
  // A special type used by analysis passes to provide an address that
  // identifies that particular analysis pass type.
  static llvm::AnalysisKey Key;
  friend struct llvm::AnalysisInfoMixin<FindInputValues>;
};

class InputValPrinter : public llvm::PassInfoMixin<InputValPrinter> {
public:
  explicit InputValPrinter(llvm::raw_ostream &OutS) : OS(OutS) {}
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);

private:
  llvm::raw_ostream &OS;
};

#endif // LLVM_TUTOR_RIV_H