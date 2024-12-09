#include "FindInputValues.h"

#include "utils.h"

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/InstructionCost.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/InstSimplifyPass.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace llvm;

std::vector<Value *> inputValues(CallInst &I, StringRef name) {
  std::vector<Value *> vals{};

  if(name == "scanf") {
    // All after first argument
    for(auto arg = I.arg_begin() + 1; arg != I.arg_end(); ++arg) {
      vals.push_back(arg->get());
    }
  } else if(name == "fscanf" || name == "sscanf") {
    // All after second argument
    for(auto arg = I.arg_begin() + 2; arg != I.arg_end(); ++arg) {
      vals.push_back(arg->get());
    }
  } else if(name == "gets" || name == "fgets" || name == "fread") {
    // first argument
    vals.push_back(I.getArgOperand(0));
  } else if(name == "getopt") {
    // third argument
    vals.push_back(I.getArgOperand(2));
  }

  // Finally, add retval
  if(auto retval = dyn_cast_if_present<Value>(&I)) {
    vals.push_back(retval);

    for(auto U: I.users()) {
      if(auto SI = dyn_cast<StoreInst>(U)) {
        vals.push_back(SI->getOperand(1));
      }
    }
  }

  return vals;
}

bool printDefUse(raw_ostream &OutS, Value &V) {
  // if(!V.hasNUsesOrMore(1)) {
  //   return false;
  // }

  for(User *U: V.users()) {

    if(isa<ICmpInst>(U) || isa<BranchInst>(U)) {
      auto *I = cast<Instruction>(U);
      OutS << "***" << V.getName() << ": (" << utils::getInstructionSrc(*I)
           << ")";
      // printInstructionSrc(OutS, *I);
      // OutS << "\t";
      return true;
      // } else if(auto SI = dyn_cast<StoreInst>(U)) {
      // LLVM_DEBUG(dbgs() << "boi");
      // LLVM_DEBUG(dbgs() << SI->getPointerOperand()->getName());
      //   // printDefUse(OutS, *SI->getValueOperand());
    } else { // if(auto *I = dyn_cast<Instruction>(U)) {
      // printInstructionSrc(OutS, *cast<Instruction>(U));
      // OutS << "\t";
    }

    // Recurse
    // printDefUse(OutS, *U);

    if(printDefUse(OutS, *U)) {
      return true;
    }

    // if(auto V_prime = dyn_cast<Value>(U)) {
    //   OutS << "--> ";
    //   if(printDefUse(OutS, *V_prime)) {
    //     // return true;
    //   }

    //   OutS << "<--\n";
    // }
  }

  return false;
}

struct InputCallVisitor : public InstVisitor<InputCallVisitor> {
  std::vector<Value *> ioVals{};
  FunctionAnalysisManager &FAM;

  InputCallVisitor(Module &M, ModuleAnalysisManager &MAM)
      : FAM{MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager()} {
        };

  void visitCallInst(CallInst &CI) {
    if(auto *F = CI.getCalledFunction()) {
      auto TLI = FAM.getResult<TargetLibraryAnalysis>(*F);
      StringRef name;
      if(isLibFunc(*F, TLI) && isInputFunc(*F, name)) {
        auto vals = inputValues(CI, name);
        ioVals.insert(ioVals.end(), vals.begin(), vals.end());
      }
    }
  }
};

bool usesInputVal(Instruction &I, std::vector<Value *> &inputValues) {
  for(Use &U: I.operands()) {
    if(auto V = dyn_cast<Value>(U)) {
      // Check if in input values
      if(std::find(inputValues.cbegin(), inputValues.cend(), V) !=
         inputValues.cend()) {
        errs() << "FOUNDIT";
        utils::printInstructionSrc(errs(), cast<Instruction>(*U));
        return true;
      } else if(auto uI = dyn_cast<Instruction>(U)) {
        errs() << "sadge:";
        utils::printInstructionSrc(errs(), *uI);
        if(usesInputVal(*uI, inputValues)) {
          return true;
        }
      }
    }
  }

  return false;
}

FindInputValues::Result FindInputValues::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  //   DominatorTree *DT = &FAM.getResult<DominatorTreeAnalysis>(F);
  Result Res = FindInputValues::Result{};
  InputCallVisitor ICV(M, MAM);

  ICV.visit(&M);

  Res.inputVals = ICV.ioVals;
  return Res;
}

FindInputReturnFunctions::Result
FindInputReturnFunctions::run(Function &F, FunctionAnalysisManager &FAM) {
  Result Res = FindInputReturnFunctions::Result{};
  if(F.getName() == "main") {
    errs() << "Skipping main function...\n";
    Res.returnIsInput = false;
    return Res;
  }

  const auto M = F.getParent();
  const auto &MAMProxy = FAM.getResult<ModuleAnalysisManagerFunctionProxy>(F);

  // assert(MAMProxy.cachedResultExists<FindInputValues>(*F.getParent()) &&
  //        "This pass need module analysis FindInputValues!");
  auto inputValues = MAMProxy.getCachedResult<FindInputValues>(*M)->inputVals;

  for(auto &BB: F) {
    for(auto &I: BB) {
      if(isa<ReturnInst>(I)) {
        Res.returnIsInput = usesInputVal(I, inputValues);
      }
    }
  }

  return Res;
}

PreservedAnalyses InputValPrinter::run(Module &M, ModuleAnalysisManager &MAM) {
  auto inputVals = MAM.getResult<FindInputValues>(M).inputVals;

  // HACK
  auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  for(auto &F: M) {
    FAM.getResult<FindInputReturnFunctions>(F);
  }

  for(auto V: inputVals) {
    // OS << V->getName() << ":";
    // if (auto *I = dyn_cast<Instruction>(V)) {
    //   printInstructionSrc(OS, *I);
    // }
    if(printDefUse(OS, *V)) {
      OS << "\n\t";
      // find if storeinst uses V
      for(auto U: V->users()) {
        if(auto SI = dyn_cast<StoreInst>(U)) {
          OS << "STORE: " << utils::getInstructionSrc(*SI);
        } else {
          OS << "NO STORE:";
          if(auto I = dyn_cast<Instruction>(U)) {
            OS << utils::getInstructionSrc(*I);
          }
        }
        errs() << "\n\t";
      }
      errs() << "---\n";
    }
  }

  return PreservedAnalyses::all();
}

AnalysisKey FindInputValues::Key;
AnalysisKey FindInputReturnFunctions::Key;
llvm::PassPluginLibraryInfo getInputValPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION,
          "input-vals",
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "opt -passes=print<input-vals>"
            PB.registerPipelineParsingCallback(
                [&](StringRef Name,
                    ModulePassManager &MPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  if(Name == "print<input-vals>") {
                    MPM.addPass(InputValPrinter(llvm::errs()));
                    return true;
                  }
                  return false;
                });
            // #2 REGISTRATION FOR "MAM.getResult<FindInputValues>(Module)"
            PB.registerAnalysisRegistrationCallback(
                [](ModuleAnalysisManager &MAM) {
                  MAM.registerPass([&] { return FindInputValues(); });
                });
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return FindInputReturnFunctions(); });
                });
          }};
};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInputValPluginInfo();
}