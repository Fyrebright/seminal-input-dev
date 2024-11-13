#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <ostream>
#include <set>
#include <vector>

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

// const std::map<StringRef, int> INPUT_FUNC_TYPES {
//     {"fdopen", 0}, {"fgetc", 0},         {"fgetpos", 0}, {"fgets", 0},
//     {"fopen", 0},  {"fread", 2},         {"freopen", 0}, {"fscanf", 0},
//     {"getc", 0},   {"getc_unlocked", 0}, {"getchar", 0}, {"getchar_unlocked",
//     0},
//     {"getopt", 0}, {"gets", 0},          {"getw", 0},    {"popen", 0},
//     {"scanf", 1},  {"sscanf", 0},        {"ungetc", 0},
// };

std::istream &GotoLine(std::istream &file, unsigned int num) {
  file.seekg(std::ios::beg);
  for(int i = 0; i < num - 1; ++i) {
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return file;
}

unsigned int lineNum(Instruction &I) {
  if(DILocation *Loc = I.getDebugLoc())
    return Loc->getLine();
  else
    return 0;
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
    errs() << "Line " << Line << " source: " << line << "("
           << I.getOpcodeName()
           //  << "," << I.getOperand(1)->getName()
           << ")\n";

    srcFile.close();
  }
}

void printUseDef(Instruction &I) {
  for(Use &U: I.operands()) {
    Value *V = U.get();
    if(auto *I = dyn_cast<Instruction>(V)) {
      // https://stackoverflow.com/questions/74927170/find-pointer-to-the-instruction-that-assigns-to-value-in-llvm

      printInstructionSrc(*I);
    } else {
      errs() << "Not an instruction??\n";
    }
    // v->print(errs());
    // v->printAsOperand(errs());
    // errs() << v->getType();
  }
}

void printDefUse(Value &V) {
  for(User *U: V.users()) {
    if(Instruction *I = dyn_cast_if_present<Instruction>(U)) {
      printInstructionSrc(*I);
    }
    if(isa<ICmpInst>(U) || isa<BranchInst>(U)) {
      errs() << "FOUND A SPOT WOW-------\n";
      // return;
    }

    // Recurse
    if(auto V_prime = dyn_cast<Value>(U)) {
      printDefUse(*V_prime);
    }
  }
}

bool isLibFunc(Function &F, TargetLibraryInfo &TLI) {
  // errs() << "checking if " << F.getName() << " is a LibFunc\n";

  // if(EVIL_FUNCTIONS.find(F.getName()) != EVIL_FUNCTIONS.cend()) {
  //   return true;
  // }

  LibFunc _;
  return TLI.getLibFunc(F.getName(), _);
}

bool isInputFunc(Function &F, StringRef &name) {
  name = F.getName();

  // Remove prefix if present
  name.consume_front("__isoc99_");
  // errs() << "after consume " << name << "\n";

  return INPUT_FUNCTIONS.find(name) != INPUT_FUNCTIONS.cend();
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

    // errs() << "\n";
    for(auto U: I.users()) {
      errs() << ":u";
      // if(auto *inst = dyn_cast<Instruction>(U)) {
      //   printInstructionSrc(*inst);
      // }

      if(auto SI = dyn_cast<StoreInst>(U)) {
        vals.push_back(SI->getOperand(1));
      }
    }
  }

  return vals;
}

struct KeyPointVisitor : public InstVisitor<KeyPointVisitor> {
  std::set<Instruction *> conditionalBranches{};
  FunctionAnalysisManager &FAM;

  KeyPointVisitor(Module &M, ModuleAnalysisManager &MAM)
      : FAM{MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager()} {
        };

  // void visitIndirectBrInst(IndirectBrInst &I)                { errs() <<
  // "IndirectBrInst(" << lineNum(I) << ")\n"; } void visitBranchInst(BranchInst
  // &I)                { errs() << "BranchInst(" << lineNum(I) << ")\n"; } void
  // visitSwitchInst(SwitchInst &I)                { errs() << "SwitchInst(" <<
  // lineNum(I) << ")\n"; } void visitCallInst(CallInst &I)                {
  // errs() << "CallInst(" << lineNum(I) << ")\n"; } void
  // visitInvokeInst(InvokeInst &I)            { errs() << "InvokeInst(" <<
  // lineNum(I) << ")\n"; } void visitCallBrInst(CallBrInst &I)            {
  // errs() << "CallBrInst(" << lineNum(I) << ")\n"; }

  // void visitICmpInst(ICmpInst &I)                { errs() << "ICmpInst(" <<
  // lineNum(I) << ")\n";} void visitFCmpInst(FCmpInst &I)                {
  // errs() << "FCmpInst(" << lineNum(I) << ")\n";} void
  // visitAllocaInst(AllocaInst &I)            { errs() << "AllocaInst(" <<
  // lineNum(I) << ")\n";} void visitLoadInst(LoadInst     &I)            {
  // errs() << "LoadInst(" << lineNum(I) << ")\n";} void
  // visitStoreInst(StoreInst   &I)            { errs() << "StoreInst(" <<
  // lineNum(I) << ")\n";} void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I) {
  // errs() << "AtomicCmpXchgInst(" << lineNum(I) << ")\n";} void
  // visitAtomicRMWInst(AtomicRMWInst &I)      { errs() << "AtomicRMWInst(" <<
  // lineNum(I) << ")\n";} void visitFenceInst(FenceInst   &I)            {
  // errs() << "FenceInst(" << lineNum(I) << ")\n";} void
  // visitGetElementPtrInst(GetElementPtrInst &I){ errs() <<
  // "GetElementPtrInst(" << lineNum(I) << ")\n";} void visitPHINode(PHINode &I)
  // { errs() << "PHINode(" << lineNum(I) << ")\n";} void
  // visitTruncInst(TruncInst &I)              { errs() << "TruncInst(" <<
  // lineNum(I) << ")\n";} void visitZExtInst(ZExtInst &I)                {
  // errs() << "ZExtInst(" << lineNum(I) << ")\n";} void visitSExtInst(SExtInst
  // &I)                { errs() << "SExtInst(" << lineNum(I) << ")\n";} void
  // visitFPTruncInst(FPTruncInst &I)          { errs() << "FPTruncInst(" <<
  // lineNum(I) << ")\n";} void visitFPExtInst(FPExtInst &I)              {
  // errs() << "FPExtInst(" << lineNum(I) << ")\n";} void
  // visitFPToUIInst(FPToUIInst &I)            { errs() << "FPToUIInst(" <<
  // lineNum(I) << ")\n";} void visitFPToSIInst(FPToSIInst &I)            {
  // errs() << "FPToSIInst(" << lineNum(I) << ")\n";} void
  // visitUIToFPInst(UIToFPInst &I)            { errs() << "UIToFPInst(" <<
  // lineNum(I) << ")\n";} void visitSIToFPInst(SIToFPInst &I)            {
  // errs() << "SIToFPInst(" << lineNum(I) << ")\n";} void
  // visitPtrToIntInst(PtrToIntInst &I)        { errs() << "PtrToIntInst(" <<
  // lineNum(I) << ")\n";} void visitIntToPtrInst(IntToPtrInst &I)        {
  // errs() << "IntToPtrInst(" << lineNum(I) << ")\n";} void
  // visitBitCastInst(BitCastInst &I)          { errs() << "BitCastInst(" <<
  // lineNum(I) << ")\n";} void visitAddrSpaceCastInst(AddrSpaceCastInst &I) {
  // errs() << "AddrSpaceCastInst(" << lineNum(I) << ")\n";} void
  // visitSelectInst(SelectInst &I)            { errs() << "SelectInst(" <<
  // lineNum(I) << ")\n";} void visitVAArgInst(VAArgInst   &I)            {
  // errs() << "VAArgInst(" << lineNum(I) << ")\n";} void
  // visitExtractElementInst(ExtractElementInst &I) { errs() <<
  // "ExtractElementInst(" << lineNum(I) << ")\n";} void
  // visitInsertElementInst(InsertElementInst &I) { errs() <<
  // "InsertElementInst(" << lineNum(I) << ")\n";} void
  // visitShuffleVectorInst(ShuffleVectorInst &I) { errs() <<
  // "ShuffleVectorInst(" << lineNum(I) << ")\n";} void
  // visitExtractValueInst(ExtractValueInst &I){ errs() << "ExtractValueInst("
  // << lineNum(I) << ")\n";} void visitInsertValueInst(InsertValueInst &I)  {
  // errs() << "InsertValueInst(" << lineNum(I) << ")\n"; } void
  // visitLandingPadInst(LandingPadInst &I)    { errs() << "LandingPadInst(" <<
  // lineNum(I) << ")\n"; } void visitFuncletPadInst(FuncletPadInst &I) { errs()
  // << "FuncletPadInst(" << lineNum(I) << ")\n"; } void
  // visitCleanupPadInst(CleanupPadInst &I) { errs() << "CleanupPadInst(" <<
  // lineNum(I) << ")\n"; } void visitCatchPadInst(CatchPadInst &I)     { errs()
  // << "CatchPadInst(" << lineNum(I) << ")\n"; } void
  // visitFreezeInst(FreezeInst &I)         { errs() << "FreezeInst(" <<
  // lineNum(I) << ")\n"; }

  // void visitBranchInst(BranchInst &BI) {
  //   errs() << "BI found: ";
  //   printInstructionSrc(BI);
  //   // if(BI.isConditional()) {
  //   //   errs() << "\n";
  //   //   printInstructionSrc(BI);
  //   //   conditionalBranches.insert(&BI);
  //   //   errs() << "\n---\nUseDef:\n";
  //   //   printUseDef(BI);
  //   //   errs() << "\n---\n";
  //   // } else {
  //   //   errs() << "not conditional type=" << BI.getValueName() << "\n";
  //   // }
  // }

  void visitCallInst(CallInst &CI) {
    if(auto *F = CI.getCalledFunction()) {

      auto TLI = FAM.getResult<TargetLibraryAnalysis>(*F);
      StringRef name;
      if(isLibFunc(*F, TLI) && isInputFunc(*F, name)) {
        printInstructionSrc(CI);

        errs() << "arg:";
        auto inputVals = inputValues(CI, name);
        for(auto val: inputVals) {
          errs() << val->getName() << ",";
          printDefUse(*val);

          // TODO: find if input vals dominate branches (2 stage pass?)
        }
        errs() << "\n";
      }
      // Value *retVal =

      // for(auto U: CI.users()) {
      //   errs() << "u";
      //   if(auto BI = dyn_cast<BranchInst>(U)) {
      //     errs() << "\t BRANCH: ";
      //     printInstructionSrc(*BI);
      //   } else if(auto I = dyn_cast<Instruction>(U)) {

      //     errs() << "\t OTHER: ";
      //     printInstructionSrc(*I);
      //   } else {
      //     errs() << "\t???: " << U->getNameOrAsOperand() << "\n";
      //   }
      // }
    }
  }
};

struct PrintBranchPass : public PassInfoMixin<PrintBranchPass> {

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {

    // for(auto &F : M.functions()){
    //   for(auto &BB : F) {
    //     for(auto &I : BB) {
    //       printInstructionSrc(I);
    //     }
    //   }
    // }

    KeyPointVisitor KPV(M, MAM);
    KPV.visit(M);

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
            // PB.registerAnalysisRegistrationCallback(
            //   [](FunctionAnalysisManager &FAM){
            //     // FAM.registerPass()
            //   }
            // );
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(PrintBranchPass());
                });
            // PB.invokeVectorizerStartEPCallbacks(
            //     [](FunctionPassManager &FPM, OptimizationLevel Level) {
            //       FPM.addPass(PromotePass());
            //     });
          }};
}
