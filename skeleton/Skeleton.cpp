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
      // bool ImplicitCode = Loc->isImplicitCode();

      ifstream srcFile(filesystem::canonical((Dir + "/" + File).str()),
                       ios::in);

      GotoLine(srcFile, Line);
      string line;
      getline(srcFile, line);
      errs() << "\nLine " << Line << " source: " << line;

      srcFile.close();
    }
  }

  void printInstructionSrc(Instruction *I) {
    if(DILocation *Loc = I->getDebugLoc()) {
      unsigned Line = Loc->getLine();
      StringRef File = Loc->getFilename();
      StringRef Dir = Loc->getDirectory();
      // bool ImplicitCode = Loc->isImplicitCode();

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

      BasicBlock *ExitingBB = L->getExitingBlock();
      // Assumed only one exiting block.
      if(!ExitingBB)
        return;

      BranchInst *ExitingBI = dyn_cast<BranchInst>(ExitingBB->getTerminator());
      if(ExitingBI) {
        printInstructionSrc(ExitingBI);
        errs() << " <--- exiting condition\n";
        // LLVM_DEBUG(dbgs() << "" << ExitingBI->getCondition()->getName() <<
        // "\n");
        errs() << "" << ExitingBI->getNumOperands() << "\n";
        errs() << "" << ExitingBI->getOperand(0) << "\n";
      }
      return;

      // LLVM_DEBUG(dbgs() << "Is canonical??  " << L->isCanonical(SE));

      // if(auto LB = L->getBounds(SE)) {
      // LLVM_DEBUG(dbgs() << "Loop Bounds!!!\n");
      //   errs() << "\t"
      //          << "getInitialIVValue" << LB->getInitialIVValue().getName()
      //          << "\n";
      // LLVM_DEBUG(dbgs() << "\t" << "getStepInst" << LB->getStepInst() <<
      // "\n"); LLVM_DEBUG(dbgs() << "\t" << "getStepValue" <<
      // LB->getStepValue() << "\n"); LLVM_DEBUG(dbgs() << "\t" <<
      // "getFinalIVValue" << LB->getFinalIVValue() << "\n"); errs() << "\t" <<
      // "getCanonicalPredicate" << LB->getCanonicalPredicate() << "\n"; errs()
      // << "\t" << "getDirection"
      // << LB->getDirection() << "\n";
      // }

      if(L->getLoopLatch()) {
        BasicBlock *BB = L->getLoopLatch();
        errs() << "\n---\nLoop Latch:\n";
        // if(BB->getSinglePredecessor()){
        //   // BB->getSinglePredecessor()->print(errs());
        // LLVM_DEBUG(dbgs() << BB->getSinglePredecessor()->getName());
        // }
        // BB->print(errs());

        for(auto &I: *BB) {
          // I.print(errs());
          // LLVM_DEBUG(dbgs() << "Instruction: \n");
          // if (llvm::isa<llvm::BranchInst>(I)) {
          // LLVM_DEBUG(dbgs() << "BranchInst: \n");
          I.print(errs(), true);
          printInstructionSrc(I);
          // LLVM_DEBUG(dbgs() << "\n---\n");
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
      // LLVM_DEBUG(dbgs() << "\n---\nBasic block:\n");
      // B.print(errs());

      for(auto &I: B) {
        // I.print(errs());
        // LLVM_DEBUG(dbgs() << "Instruction: \n");
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

    LibFunc libF;
    for(auto &F: M.functions()) {
      const TargetLibraryInfo &TLI = FAM.getResult<TargetLibraryAnalysis>(F);
      // const TargetLibraryInfo *TLI;

      // Skip debug functions
      if(F.isIntrinsic()) {
        // LLVM_DEBUG(dbgs() << "SKIPPED: " << F.getName() << "\n");
        continue;
      }

      // rand causes crash every time...
      if(F.getName() == "rand" | F.getName() == "srand") {
        // LLVM_DEBUG(dbgs() << "Lets try rand... (");

        int isLib = TLI.getLibFunc(F.getName(), libF);
        if(!isLib)
          // LLVM_DEBUG(dbgs() << "WTF!!");
          // LLVM_DEBUG(dbgs() << TLI.getLibFunc(F.getName(), libF));

          // LLVM_DEBUG(dbgs() << ") SURVIVED??\n");

          // LLVM_DEBUG(dbgs() << "SKIPPED: " << F.getName() << "\n");
          continue;
      }

      // LLVM_DEBUG(dbgs() << "In a function called " << F.getName() << "!\n");

      // Skip library functions
      bool isLib = TLI.getLibFunc(F.getName(), libF);
      if(isLib) {
        // LLVM_DEBUG(dbgs() << "SKIPPED: " << F.getName() << "\n");
      } else {
        LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
        ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);

        //   funcDefUse(F);
        // funcBranches(F);
        funcLoopPrint(F, LI, SE);
      }

      // LLVM_DEBUG(dbgs() << "I saw a function called " << F.getName() <<
      // "!\n");
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
