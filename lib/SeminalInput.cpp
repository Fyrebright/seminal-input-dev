#include "SeminalInput.h"

#include "FindIOVals.h"
#include "FindKeyPoints.h"
#include "utils.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
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

#include <algorithm>
#include <cstdlib>
#include <ostream>
#include <string>

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "seminal-input"
#endif // DEBUG_TYPE

using namespace llvm;

static constexpr char PassArg[] = "seminal-input";
static constexpr char PluginName[] = "SeminalInput";

//------------------------------------------------------------------------------
// SeminalInput implementation
//------------------------------------------------------------------------------

bool defUseRecurse(Value &V,
                   const FindKeyPoints::Result &KeyPoints,
                   bool reset = false) {
  // Track visited vals
  if(reset) {
    static std::set<Value *> visited{};
    visited.clear();
  }
  static std::set<Value *> visited{};

  for(User *U: V.users()) {
    // if(U->getName() == "") {
    //   dbgs() << " not a var??\n";
    //   // continue;
    // } else {
    //   dbgs() << U->getName();
    // }
    // Find member of key points that has the same name
    // dbgs() << "(--";
    // U->getType()->print(dbgs());
    // U->print(dbgs());
    // dbgs() << "--),";

    auto hasSameName = [&U](Instruction *I) {
      //   if(I->getName() != "" && I->getName() == U->getName()) {
      //   }
      //   return I->getName() != "" && I->getName() == U->getName();
      return I == U;
    };

    auto findRes = std::find_if(
        KeyPoints.keyPoints.cbegin(), KeyPoints.keyPoints.cend(), hasSameName);
    if(findRes != KeyPoints.keyPoints.cend()) {

      dbgs() << "FOUND A SPOT WOW-------\n";
      //   print found key point
      auto I = (*findRes);

      //   dbgs() << "Found a key point: ";
      utils::printInstructionSrc(dbgs(), *I);

      return true;
    }

    if(auto V_prime = dyn_cast<Value>(U)) {
      if(visited.find(V_prime) != visited.cend()) {
        continue;
      }
      visited.insert(V_prime);
      return defUseRecurse(*V_prime, KeyPoints);
    }
  }
  //   dbgs() << "\n";
  return false;
}

// void searchfromInput2(Function &F,
//                       const FindIOVals::Result &IOVals,
//                       const FuncReturnIO::Result &IsRetIO,
//                       const FindKeyPoints::Result &KeyPoints) {

//   for(auto &V: IOVals.ioVals) {
//     if(defUseRecurse(*V, KeyPoints)) {
//       dbgs() << "FOUND A SEMINAL INPUT2\n";
//     } else {
//       dbgs() << "NOT FOUND A SEMINAL INPUT2\n";
//     }
//   }
// }

SeminalInput::Result searchFromInput(Function &F,
                                     const FindIOVals::Result &IOVals,
                                     const FuncReturnIO::Result &IsRetIO,
                                     const FindKeyPoints::Result &KeyPoints) {
  // // Graphing

  //   return SeminalInput::Result{};
  std::vector<IOValMetadata> semVals{};

  for(auto &ioValEntry: IOVals.ioValsMetadata) {
    auto V = ioValEntry.val;
    if(defUseRecurse(*V, KeyPoints, true)) {
      dbgs() << "FOUND A SEMINAL INPUT1\n";
      semVals.push_back(ioValEntry);
    }
  }

  return SeminalInput::Result{semVals};
}

SeminalInput::Result SeminalInput::run(Function &F,
                                       FunctionAnalysisManager &FAM) {
  // Get results of FindIOVals, FindKeyPoints, and FuncReturnIO
  auto &IOVals = FAM.getResult<FindIOVals>(F);
  auto &IsRetIO = FAM.getResult<FuncReturnIO>(F);
  auto &KeyPoints = FAM.getResult<FindKeyPoints>(F);

  auto Res = searchFromInput(F, IOVals, IsRetIO, KeyPoints);

  return Res;
}

//------------------------------------------------------------------------------
// SeminalInputGraph implementation
//------------------------------------------------------------------------------

std::string colors[] = {"red", "blue", "yellow", "purple", "orange"};

void _recurse_case(llvm::raw_ostream &OS,
                   Value &V,
                   const FindKeyPoints::Result &KeyPoints,
                   int colorIdx) {
  static std::set<Value *> visited{};

  for(User *U: V.users()) {

    // For checking if user and instruction have the same name
    auto hasSameName = [&U](Instruction *I) {
      return I == U;
    };

    auto findRes = std::find_if(
        KeyPoints.keyPoints.cbegin(), KeyPoints.keyPoints.cend(), hasSameName);
    if(findRes != KeyPoints.keyPoints.cend()) {

      OS << "\"" << *dyn_cast<Instruction>(U) << "\"" << " -> " << "\"" << V
         << "\"" << ";\n";
      OS << "\"" << *dyn_cast<Instruction>(U) << "\""
         << " [ color = green ]\n";

      return;
    }

    if(dyn_cast<Instruction>(U)) {
      OS << "\"" << *dyn_cast<Instruction>(U) << "\"" << " -> " << "\"" << V
         << "\"" << ";\n";
      OS << "\"" << *dyn_cast<Instruction>(U) << "\""
         << " [ color = " << colors[colorIdx % 5] << " ]\n";
    }

    if(auto V_prime = dyn_cast<Value>(U)) {
      if(visited.find(V_prime) != visited.cend()) {
        continue;
      }

      visited.insert(V_prime);
      _recurse_case(OS, *V_prime, KeyPoints, colorIdx);
    }
  }
}

void defUseDigraph(llvm::raw_ostream &OS,
                   Function &F,
                   const FindIOVals::Result &IOVals,
                   const FuncReturnIO::Result &IsRetIO,
                   const FindKeyPoints::Result &KeyPoints) {

  OS << "digraph " + F.getName() + "{\n";
  OS << "\n";
  int colorIdx = 0;
  for(auto &V: IOVals.ioVals) {
    OS << "\"" << *V << "\"" << " [ color = green ]\n";
    _recurse_case(OS, *V, KeyPoints, colorIdx++);
  }
  OS << "\n}\n";
}

PreservedAnalyses SeminalInputGraph::run(Function &F,
                                         FunctionAnalysisManager &FAM) {

  auto &IOVals = FAM.getResult<FindIOVals>(F);
  auto &IsRetIO = FAM.getResult<FuncReturnIO>(F);
  auto &KeyPoints = FAM.getResult<FindKeyPoints>(F);

  defUseDigraph(OS, F, IOVals, IsRetIO, KeyPoints);
  return PreservedAnalyses::all();
}

//------------------------------------------------------------------------------
// SeminalInputPrinter implementation
//------------------------------------------------------------------------------

static void printSemVals(raw_ostream &OS,
                         Function &Func,
                         const SeminalInput::Result &res) noexcept {
  //   dbgs() << "======\n";
  // Print input value in format:
  //    Line <line num>: description including name of variable
  for(const auto &semVal: res.semValsMetadata) {
    OS << "Line " << semVal.lineNum << ": " << "in \"" << semVal.funcName
       << "\" " << semVal.val->getName() << semVal.comment << "\n";
  }
}

PreservedAnalyses SeminalInputPrinter::run(Function &F,
                                           FunctionAnalysisManager &FAM) {
  auto &semVals = FAM.getResult<SeminalInput>(F);
  printSemVals(OS, F, semVals);
  return PreservedAnalyses::all();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::AnalysisKey SeminalInput::Key;

PassPluginLibraryInfo getSeminalInputPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION,
          PluginName,
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "FAM.getResult<SeminalInput>(Function)"
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return FindIOVals(); });
                  FAM.registerPass([&] { return FuncReturnIO(); });
                  FAM.registerPass([&] { return FindKeyPoints(); });
                  FAM.registerPass([&] { return SeminalInput(); });
                });
            PB.registerPipelineParsingCallback(
                [&](StringRef Name,
                    FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  std::string PrinterPassElement =
                      formatv("print<{0}>", PassArg);
                  if(!Name.compare(PrinterPassElement)) {
                    FPM.addPass(SeminalInputPrinter(llvm::outs()));
                    return true;
                  }

                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getSeminalInputPluginInfo();
}