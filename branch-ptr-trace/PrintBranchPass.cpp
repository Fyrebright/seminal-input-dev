#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>

using namespace llvm;

namespace {
/* Functions that one must not call getLibFunc on. Probably aliases */
const std::set<StringRef> EVIL_FUNCTIONS{"rand", "srand"};

const std::set<StringRef> INPUT_FUNCTIONS{
    "fdopen", "fgetc",         "fgetpos", "fgets",
    "fopen",  "fread",         "freopen", "fscanf",
    "getc",   "getc_unlocked", "getchar", "getchar_unlocked",
    "getopt", "gets",          "getw",    "popen",
    "scanf",  "sscanf",        "ungetc",
};

std::istream &GotoLine(std::istream &file, unsigned int num) {
  file.seekg(std::ios::beg);
  for(int i = 0; i < num - 1; ++i) {
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return file;
}

void printInstructionSrc(Instruction &I) {
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
    errs() << "Line " << Line << " source: " << line << "\n";

    srcFile.close();
  }
}

void printUseDef(Instruction &I) {
  for(Use &U: I.operands()) {
    Value *v = U.get();
    // v->print(errs());
    // v->printAsOperand(errs());
  }
}

bool isLibFunc(Function &F, TargetLibraryInfo &TLI) {
  // errs() << "checking if " << F.getName() << " is a LibFunc\n";

  if(EVIL_FUNCTIONS.find(F.getName()) != EVIL_FUNCTIONS.cend()) {
    return true;
  }

  LibFunc _;
  return TLI.getLibFunc(F.getName(), _);
}

bool isInputFunc(Function &F) {
  StringRef name = F.getName();

  // Remove prefix if present
  name.consume_front("__isoc99_");
  errs() << "after consume " << name << "\n";

  return INPUT_FUNCTIONS.find(name) != INPUT_FUNCTIONS.cend();
}

struct KeyPointVisitor : public InstVisitor<KeyPointVisitor> {
  std::set<Instruction *> conditionalBranches{};
  TargetLibraryInfo TLI;

  KeyPointVisitor(TargetLibraryInfo newTLI) : TLI(newTLI) {};

  void visitBranchInst(BranchInst &BI) {
    // errs() << "BI found: ";
    if(BI.isConditional()) {
      // errs() << "\n";
      printInstructionSrc(BI);
      conditionalBranches.insert(&BI);
      // errs() << "\n---\nUseDef:\n";
      // printUseDef(BI);
      // errs() << "\n---\n";
    } else {
      // errs() << "not conditional type=" << BI.getValueName() << "\n";
    }
  }

  void visitCallInst(CallInst &CI) {
    if(auto *F = CI.getCalledFunction()) {
      if(isLibFunc(*F, TLI) && isInputFunc(*F)){
        errs() << "YES\n";
        printInstructionSrc(CI);
      }
    }
  }
};

struct PrintBranchPass : public PassInfoMixin<PrintBranchPass> {

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    auto &FAM =
        MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    LibFunc libF;
    for(auto &F: M.functions()) {
      const TargetLibraryInfo &TLI = FAM.getResult<TargetLibraryAnalysis>(F);

      errs() << F.getName() << "\n";
      KeyPointVisitor BIV(TLI);
      BIV.visit(F);
      errs() << "found " << BIV.conditionalBranches.size()
             << " conditional branches\n";
    }

    return PreservedAnalyses::all();
  }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "PrintBranches",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(PrintBranchPass());
                });
          }};
}
