#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "FindInputValues.h"

using namespace llvm;

FindInputValues::Result FindInputValues::run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {
//   DominatorTree *DT = &FAM.getResult<DominatorTreeAnalysis>(F);
  Result Res = FindInputValues::Result{};

  return Res;
}

PreservedAnalyses InputValPrinter::run(Function &Func,
                                  FunctionAnalysisManager &FAM) {

  auto inputMap = FAM.getResult<FindInputValues>(Func);

  // TODO: print
  errs() << "It worked...\n";
  return PreservedAnalyses::all();
}

AnalysisKey FindInputValues::Key;
llvm::PassPluginLibraryInfo getInputValPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "input-vals", LLVM_VERSION_STRING, [](PassBuilder &PB) {
        // #1 REGISTRATION FOR "opt -passes=print<input-vals>"
        PB.registerPipelineParsingCallback(
            [&](StringRef Name,
                FunctionPassManager &FPM,
                ArrayRef<PassBuilder::PipelineElement>) {
              if(Name == "print<input-vals>") {
                FPM.addPass(InputValPrinter(llvm::errs()));
                return true;
              }
              return false;
            });
        // #2 REGISTRATION FOR "FAM.getResult<FindInputValues>(Function)"
        PB.registerAnalysisRegistrationCallback(
            [](FunctionAnalysisManager &FAM) {
              FAM.registerPass([&] { return FindInputValues(); });
            });
      }};
};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInputValPluginInfo();
}