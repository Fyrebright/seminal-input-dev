#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/User.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

using namespace llvm;
using namespace std;

istream &GotoLine(istream &file, unsigned int num) {
  file.seekg(ios::beg);
  for(int i = 0; i < num - 1; ++i) {
    file.ignore(numeric_limits<streamsize>::max(), '\n');
  }
  return file;
}

namespace {
struct SkeletonPass : public PassInfoMixin<SkeletonPass> {
  void printInstructionSrc(Instruction &I) {
    if(DILocation *Loc = I.getDebugLoc()) {
      unsigned Line = Loc->getLine();
      StringRef File = Loc->getFilename();
      StringRef Dir = Loc->getDirectory();
      bool ImplicitCode = Loc->isImplicitCode();

      ifstream srcFile(filesystem::canonical((Dir + "/" + File).str()),
                       ios::in);

      GotoLine(srcFile, Line);
      string line;
      getline(srcFile, line);
      errs() << "\nLine " << Line << " source: " << line;

      srcFile.close();
    }
  }

  void funcLoopPrint(Function &F, LoopInfo &LI, ScalarEvolution &SE) {

    for(Loop *L: LI) {

      errs() << "Is canonical??  " << L->isCanonical(SE);

      if(auto LB = L->getBounds(SE)) {
        errs() << "Loop Bounds!!!\n";
        errs() << "\t"
               << "getInitialIVValue" << LB->getInitialIVValue().getName()
               << "\n";
        // errs() << "\t" << "getStepInst" << LB->getStepInst() << "\n";
        // errs() << "\t" << "getStepValue" << LB->getStepValue() << "\n";
        // errs() << "\t" << "getFinalIVValue" << LB->getFinalIVValue() << "\n";
        // errs() << "\t" << "getCanonicalPredicate" <<
        // LB->getCanonicalPredicate() << "\n"; errs() << "\t" << "getDirection"
        // << LB->getDirection() << "\n";
      }

      if(L->getLoopLatch()) {
        BasicBlock *BB = L->getLoopLatch();
        errs() << "\n---\nLoop Latch:\n";
        // if(BB->getSinglePredecessor()){
        //   // BB->getSinglePredecessor()->print(errs());
        //   errs() << BB->getSinglePredecessor()->getName();
        // }
        // BB->print(errs());

        for(auto &I: *BB) {
          // I.print(errs());
          // errs() << "Instruction: \n";
          // if (llvm::isa<llvm::BranchInst>(I)) {
          // errs() << "BranchInst: \n";
          I.print(errs(), true);
          printInstructionSrc(I);
          // errs() << "\n---\n";
          // }
        }

        errs() << BB->getName();
        errs() << "\n";
      }
    }
  }

  void funcDefUse(Function &F) {
    for(User *U: F.users()) {
      if(Instruction *I = dyn_cast<Instruction>(U)) {
        errs() << "F is used in instruction:\n";
        errs() << *I << "\n";
      }
    }
  }

  void funcAllOpCodes(Function &F) {
    string output;
    for(auto &B: F) {
      for(auto &I: B) {
        output += (I.getOpcodeName());
        output += '\n';
      }
    }
    errs() << output << '\n';
  }

  void funcBranches(Function &F) {
    for(auto &B: F) {
      // errs() << "\n---\nBasic block:\n";
      // B.print(errs());

      for(auto &I: B) {
        // I.print(errs());
        // errs() << "Instruction: \n";
        if(llvm::isa<llvm::BranchInst>(I)) {
          errs() << "BranchInst: \n";
          I.print(errs(), true);
          printInstructionSrc(I);
          errs() << "\n---\n";
        }
      }
    }
  }

  void funcTerminator(Function &F) {
    for(auto &B: F) {
      errs() << "\n---\nBasic block:\n";
      // B.print(errs());

      if(B.getTerminator())
        errs() << "Terminator " << B.getTerminator()->getName() << "!\n";
      else
        B.print(errs());
    }
  }

  PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM) {
    errs() << "FOFF";
    return PreservedAnalyses::all();
  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    FunctionAnalysisManager &FAM =
        MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    LibFunc _;
    for(auto &F: M.functions()) {
      const TargetLibraryInfo &TLI = FAM.getResult<TargetLibraryAnalysis>(F);
      // const TargetLibraryInfo *TLI;

      // Skip debug functions
      if(F.isIntrinsic()) {
        errs() << "SKIPPED: " << F.getName() << "\n";
        continue;
      }

      // rand causes crash every time...
      if(F.getName() == "rand") {
        errs() << "SKIPPED: " << F.getName() << "\n";
        continue;
      }

      errs() << "In a function called " << F.getName() << "!\n";

      // Skip library functions
      if(TLI.getLibFunc(F.getName(), _)) {
        errs() << "SKIPPED: " << F.getName() << "\n";
      } else {
        LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
        ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);

        //   funcDefUse(F);
        // funcBranches(F);
        funcLoopPrint(F, LI, SE);
      }

      errs() << "I saw a function called " << F.getName() << "!\n";
    }

    return PreservedAnalyses::all();
  }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Skeleton",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(SkeletonPass());
                });
          }};
}
