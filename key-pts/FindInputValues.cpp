#include "FindInputValues.h"

#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/InstructionCost.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace llvm;

std::istream &GotoLine(std::istream &file, unsigned int num) {
  file.seekg(std::ios::beg);
  for(int i = 0; i < num - 1; ++i) {
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return file;
}

void printInstructionSrc(raw_ostream &OutS, Instruction &I) {
  if(DILocation *Loc = I.getDebugLoc()) {
    unsigned Line = Loc->getLine();
    StringRef File = Loc->getFilename();
    StringRef Dir = Loc->getDirectory();
    // bool ImplicitCode = Loc->isImplicitCode();

    std::ifstream srcFile(std::filesystem::canonical((Dir + "/" + File).str()),
                          std::ios::in);

    GotoLine(srcFile, Line);
    std::string line;
    getline(srcFile, line);
    OutS << "Line " << Line << " source: " << line << "("
         << I.getOpcodeName()
         //  << "," << I.getOperand(1)->getName()
         << ")\n";

    srcFile.close();
  }
}

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
    vals.push_back(retval); // TODO: possibly need

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
      OutS << "***";
      printInstructionSrc(OutS, *I);
      // OutS << "\t";
      return true;
    // } else if(auto SI = dyn_cast<StoreInst>(U)) {
    //   errs() << "boi";
    //   errs() << SI->getPointerOperand()->getName();
    //   // printDefUse(OutS, *SI->getValueOperand());
    } else {//if(auto *I = dyn_cast<Instruction>(U)) {
      printInstructionSrc(OutS, *cast<Instruction>(U));
      OutS << "\t";
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
  std::vector<Value *> moduleInputValues{};
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
        moduleInputValues.insert(
            moduleInputValues.end(), vals.begin(), vals.end());
      }
    }
  }
};

FindInputValues::Result FindInputValues::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  //   DominatorTree *DT = &FAM.getResult<DominatorTreeAnalysis>(F);
  Result Res = FindInputValues::Result{};
  InputCallVisitor ICV(M, MAM);

  ICV.visit(&M);

  return ICV.moduleInputValues;
}

PreservedAnalyses InputValPrinter::run(Module &M, ModuleAnalysisManager &MAM) {
  auto inputVals = MAM.getResult<FindInputValues>(M);

  for(auto V: inputVals) {
    OS << V->getName() << ":\n\t";
    // if (auto *I = dyn_cast<Instruction>(V)) {
    //   printInstructionSrc(OS, *I);
    // }
    printDefUse(OS, *V);
    OS << "\n";
  }

  OS << "FFFFFFFFFFFFFFFFFFFFFF\n";

  return PreservedAnalyses::all();
}

AnalysisKey FindInputValues::Key;
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
          }};
};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInputValPluginInfo();
}