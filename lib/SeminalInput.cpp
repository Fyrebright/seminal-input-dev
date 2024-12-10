#include "SeminalInput.h"

#include "FindIOVals.h"
#include "FindKeyPoints.h"
#include "utils.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/GraphWriter.h"
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
                   std::set<Value *> &visited) {
  bool res = false;

  for(auto U: V.users()) {

    // Check if user is a key point
    auto findRes =
        std::find(KeyPoints.keyPoints.cbegin(), KeyPoints.keyPoints.cend(), U);
    if(findRes != KeyPoints.keyPoints.cend()) {
      res = true;
    }

    if(auto V_prime = dyn_cast<Value>(U)) {
      if(visited.find(V_prime) != visited.cend()) {
        continue;
      }

      // Add to visited set
      visited.insert(V_prime);
      res = res || defUseRecurse(*V_prime, KeyPoints, visited);
    }
  }
  return res;
}

SeminalInput::Result searchFromInput(Function &F,
                                     const FindIOVals::Result &IOVals,
                                     const FuncReturnIO::Result &IsRetIO,
                                     const FindKeyPoints::Result &KeyPoints) {
  std::vector<IOValMetadata> semVals{};
  std::set<Value *> visited{};

  // Recurse over def-use chain of each input value
  for(auto &ioValEntry: IOVals.ioValsMetadata) {
    auto V = ioValEntry.val;
    if(defUseRecurse(*V, KeyPoints, visited)) {
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

inline std::string _getValName(Value &V) {
  // Create stringref from ostream
  auto sep = '~';
  std::string str;
  raw_string_ostream rso(str);
  V.print(rso, false);
  rso.flush();
  StringRef valStr = str;
  std::string ret;
  if(!valStr.contains(sep)) {
    return DOT::EscapeString(valStr.trim().str());
  }

  return DOT::EscapeString(valStr.split(sep).first.trim().str());
}

void _makeEdge(Value &from, Value &to, raw_ostream &OS, int idx) {
  std::string fromName = _getValName(from);
  std::string toName = _getValName(to);

  OS << formatv("\"{0}{1}\" -> \"{0}{2}\";\n", idx, fromName, toName);
}
void _makeNode(Value &label, StringRef style, raw_ostream &OS, int idx) {
  std::string labelName = _getValName(label);
  OS << formatv("\"{0}{1}\" [{2} label=\"{1}\"];\n", idx, labelName, style);
}

void recurseGraph(llvm::raw_ostream &OS,
                  Value &V,
                  const FindKeyPoints::Result &KeyPoints,
                  int colorIdx,
                  std::set<Value *> &visited) {
  for(auto U: V.users()) {
    auto findRes =
        std::find(KeyPoints.keyPoints.begin(), KeyPoints.keyPoints.end(), U);
    if(findRes != KeyPoints.keyPoints.cend()) {
      _makeEdge(*U, V, OS, colorIdx);
      _makeNode(*U, "color = green,  style=filled", OS, colorIdx);

      return;
    }

    if(dyn_cast<Instruction>(U)) {
      _makeEdge(*U, V, OS, colorIdx);

      _makeNode(
          *U, Twine("color = ", colors[colorIdx % 5]).str(), OS, colorIdx);
    }

    if(auto V_prime = dyn_cast<Value>(U)) {
      if(visited.find(V_prime) != visited.cend()) {
        continue;
      }

      visited.insert(V_prime);
      recurseGraph(OS, *V_prime, KeyPoints, colorIdx, visited);
    }
  }
}

void defUseDigraph(llvm::raw_ostream &OS,
                   Function &F,
                   const FindIOVals::Result &IOVals,
                   const FuncReturnIO::Result &IsRetIO,
                   const FindKeyPoints::Result &KeyPoints) {

  std::set<Value *> visited{};

  OS << "digraph " + F.getName() + "{\nnode[shape=box]\n";
  int colorIdx = 0;
  for(auto &V: IOVals.ioVals) {
    OS << "subgraph cluster_" << colorIdx << " {\n";
    _makeNode(*V, "color = green,  style=filled", OS, colorIdx);
    recurseGraph(OS, *V, KeyPoints, colorIdx++, visited);
    OS << "}\n";
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
  // Print input value in format:
  //    Line <line num>: description including name of variable
  bool cmdArgs = false;
  for(const auto &semVal: res.semValsMetadata) {
    if(semVal.isFile) {
      OS << formatv("Line {0}: in func \"{1}\", contents of file {2}\n",
                    semVal.lineNum,
                    semVal.funcName,
                    semVal.comment);
    } else if(semVal.inst == nullptr && semVal.funcName == "main") {
      if(cmdArgs)
        continue;

      OS << "Command line arguments (argc/argv)\n";
      cmdArgs = true;
    } else {
      OS << formatv("Line {0}: in func \"{1}\", {2}\n",
                    semVal.lineNum,
                    semVal.funcName,
                    semVal.val->getName());
    }
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
            // printer pass
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
            // graph pass
            PB.registerPipelineParsingCallback(
                [&](StringRef Name,
                    FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  std::string GraphPassElement = formatv("graph<{0}>", PassArg);
                  if(!Name.compare(GraphPassElement)) {
                    FPM.addPass(SeminalInputGraph(llvm::outs()));
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