/**
 * @file FindInputValues.h
 * @brief 
 * @see \link https://github.com/banach-space/llvm-tutor \endlink
 */
#ifndef LLVM_TUTOR_RIV_H
#define LLVM_TUTOR_RIV_H

#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"


struct FindInputValues : public llvm::AnalysisInfoMixin<FindInputValues> {
  /**
   * @brief Map each function to values dependent on input
   */
  using Result = llvm::MapVector<llvm::Function const *,
                                 llvm::SmallPtrSet<llvm::Value *, 8>>;
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &);

private:
  // A special type used by analysis passes to provide an address that
  // identifies that particular analysis pass type.
  static llvm::AnalysisKey Key;
  friend struct llvm::AnalysisInfoMixin<FindInputValues>;
};


class InputValPrinter : public llvm::PassInfoMixin<InputValPrinter> {
public:
  explicit InputValPrinter(llvm::raw_ostream &OutS) : OS(OutS) {}
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};

#endif // LLVM_TUTOR_RIV_H