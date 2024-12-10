#include "FindKeyPoints.h"

#include "utils.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <string>

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "find-key-pts"
#endif // DEBUG_TYPE

using namespace llvm;

static constexpr char PassArg[] = "find-key-pts";
static constexpr char PluginName[] = "FindKeyPoints";

//------------------------------------------------------------------------------
// FindKeyPoints implementation
//------------------------------------------------------------------------------

bool usedByBranch(Instruction &I) {
  for(User *U: I.users()) {
    if(auto uI = dyn_cast<Instruction>(U)) {
      if(isa<BranchInst>(uI)) {
        return true;
      }
    }
  }

  return false;
}

struct KeyPointVisitor : public InstVisitor<KeyPointVisitor> {
  FindKeyPoints::Result res{};

  void visitBranchInst(BranchInst &I) {
    // LLVM_DEBUG(dbgs() << "BI found (" << utils::lineNum(I) << "): ");
    if(I.isConditional()) {
      // LLVM_DEBUG(dbgs() << "\n");
      //   utils::printInstructionSrc(errs(), I);
      res.keyPoints.insert(&I);
    } else {
      // LLVM_DEBUG(dbgs() << "not conditional type=" << I.getValueName() <<
      // "\n");
    }
  }

  void visitSwitchInst(SwitchInst &I) {
    if(I.getCondition()) {
      // Not sure if there is another case here
      res.keyPoints.insert(&I);
    } else {
      // Just in case, I want to know
      errs() << "\n***ERROR***\n";
      errs() << "SwitchInst(" << utils::lineNum(I) << "): no condition\n";
      errs() << "***********\n\n";
      exit(1111);
    }
  }

  //// TODO: these may be needed
  void visitCallBrInst(CallBrInst &I) {
    // LLVM_DEBUG(dbgs() << "CallBrInst(" << utils::lineNum(I) << ")\n";);
  }
  void visitICmpInst(ICmpInst &I) {
    // LLVM_DEBUG(dbgs() << "ICmpInst(" << utils::lineNum(I) << ")\n";);
    if(usedByBranch(I)) {
      res.keyPoints.insert(&I);
    }
  }
  void visitFCmpInst(FCmpInst &I) {
    // LLVM_DEBUG(dbgs() << "FCmpInst(" << utils::lineNum(I) << ")\n";);
    if(usedByBranch(I)) {
      res.keyPoints.insert(&I);
    }
  }

  void visitCallInst(CallInst &I) {
    // Check if callee is a function pointer
    //// NOTE: also could check if can get function name
    ////       if not, then it's a function pointer
    if(I.isIndirectCall()) {
      res.keyPoints.insert(&I);
    }
  }
};

FindKeyPoints::Result FindKeyPoints::run(Function &F,
                                         FunctionAnalysisManager &FAM) {
  Result Res = FindKeyPoints::Result{};
  KeyPointVisitor KPV{};

  KPV.visit(F);

  return KPV.res;
}

//------------------------------------------------------------------------------
// FindKeyPointsPrinter implementation
//------------------------------------------------------------------------------

static void printKeyPoints(raw_ostream &OS,
                           Function &Func,
                           const FindKeyPoints::Result &res) noexcept {

  OS << "Key Points in \"" << Func.getName() << "\":";
  if(res.keyPoints.empty()) {
    OS << " None\n";
    return;
  } else {
    OS << "\n";
  }

  // Using a ModuleSlotTracker for printing makes it so full function analysis
  // for slot numbering only occurs once instead of every time an instruction
  // is printed.
  //// TODO: possibly make use of
  // ModuleSlotTracker Tracker(Func.getParent());

  // Print instruction for each value in res.KeyPointsMetadata
  for(const auto &I: res.keyPoints) {
    OS << "\t" << utils::getInstructionSrc(*I) << "\n";

    // Print operands of instruction
    // LLVM_DEBUG(
    OS << "\t\tOperands: ";
    for(Use &U: I->operands()) {
      OS << U->getName() << ", ";
    }
    OS << "\n";
    // );
  }
}

PreservedAnalyses FindKeyPointsPrinter::run(Function &F,
                                            FunctionAnalysisManager &FAM) {
  auto &KeyPoints = FAM.getResult<FindKeyPoints>(F);
  printKeyPoints(OS, F, KeyPoints);
  return PreservedAnalyses::all();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::AnalysisKey FindKeyPoints::Key;

PassPluginLibraryInfo getFindKeyPointsPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION,
          PluginName,
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "FAM.getResult<FindKeyPoints>(Function)"
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return FindKeyPoints(); });
                });
            // #2 REGISTRATION FOR "opt -passes=print<find-key-pts>"
            // Printing passes format their pipeline element argument to the
            // pattern `print<pass-name>`. This is the pattern we're checking
            // for here.
            PB.registerPipelineParsingCallback(
                [&](StringRef Name,
                    FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  std::string PrinterPassElement =
                      formatv("print<{0}>", PassArg);
                  if(!Name.compare(PrinterPassElement)) {
                    FPM.addPass(FindKeyPointsPrinter(llvm::outs()));
                    return true;
                  }

                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFindKeyPointsPluginInfo();
}