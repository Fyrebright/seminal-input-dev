#include "FindIOVals.h"

#include "utils.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
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
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <string>

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "find-io-vals"
#endif // DEBUG_TYPE

using namespace llvm;

// Deprecated? Would like if I could include a description:
// static constexpr char PassName[] = "IO-dependent value locator";

static constexpr char PassArg[] = "find-io-val";
static constexpr char PluginName[] = "FindIOVals";

//------------------------------------------------------------------------------
// FindIOVals implementation
//------------------------------------------------------------------------------

FindIOVals::Result inputValues(CallInst &I, StringRef name) {
  std::set<Value *> vals{};
  std::vector<IOValMetadata> valsMetadata{};
  bool isFile = false;
  StringRef retvalName;

  // Get line number
  auto lineNum = utils::lineNum(I);

  if(name == "scanf") {
    // All after first argument
    for(auto arg = I.arg_begin() + 1; arg != I.arg_end(); ++arg) {
      vals.insert(arg->get());
      valsMetadata.push_back(IOValMetadata{.val = arg->get(),
                                           .inst = &I,
                                           .funcName = name.str(),
                                           .lineNum = lineNum});
    }
  } else if(name == "fscanf" || name == "sscanf") {
    // All after second argument
    for(auto arg = I.arg_begin() + 2; arg != I.arg_end(); ++arg) {
      vals.insert(arg->get());

      valsMetadata.push_back(IOValMetadata{.val = arg->get(),
                                           .inst = &I,
                                           .funcName = name.str(),
                                           .lineNum = lineNum});
    }
  } else if(name == "gets" || name == "fgets" || name == "fread") {
    // first argument
    vals.insert(I.getArgOperand(0));
    valsMetadata.push_back(IOValMetadata{.val = I.getArgOperand(0),
                                         .inst = &I,
                                         .funcName = name.str(),
                                         .lineNum = lineNum});
  } else if(name == "getopt") {
    // third argument
    vals.insert(I.getArgOperand(2));
    valsMetadata.push_back(IOValMetadata{.val = I.getArgOperand(2),
                                         .inst = &I,
                                         .funcName = name.str(),
                                         .lineNum = lineNum});
  } else if(name == "fopen" || name == "fdopen" || name == "freopen" ||
            name == "popen") {
    // Retvals, and mark as file descriptors
    isFile = true;
    auto retval = dyn_cast_if_present<Value>(&I);
    vals.insert(retval);
    valsMetadata.push_back(IOValMetadata{
        .val = retval,
        .inst = &I,
        .funcName = name.str(),
        .isFile = true,
        .comment = utils::getLhsName(I),
        .lineNum = lineNum,
    });

    retvalName = retval->getName();

  } else if(name == "fgetc" || name == "getc" || name == "getc_unlocked" ||
            name == "getchar" || name == "getchar_unlocked" || name == "getw" ||
            name == "getenv") {
    // Retvals, not file descriptors
    auto retval = dyn_cast_if_present<Value>(&I);
    vals.insert(retval);
    valsMetadata.push_back(IOValMetadata{
        .val = retval, .inst = &I, .funcName = name.str(), .lineNum = lineNum});
  }

  // Add store instructions that use the return value
  // errs() << "Users of : " << I.getName() << "\n";
  for(auto U: dyn_cast<Value>(&I)->users()) {
    // errs() << "User: " << U->getName() << "(" <<
    // dyn_cast<Instruction>(U)->getOpcodeName() << ")\n";
    if(auto SI = dyn_cast<StoreInst>(U)) {
      // errs() << "retvalName: " << retvalName << "\n";
      vals.insert(SI->getOperand(1));
      auto meta = IOValMetadata{
          .val = SI->getOperand(1),
          .inst = SI,
          .funcName = name.str(),
          .isFile = isFile,
          .lineNum = lineNum,
      };

      // If is a file descriptor, add comment to save name
      if(isFile) {
        meta.comment = retvalName.str();
      }

      valsMetadata.push_back(meta);
    }
  }


  return FindIOVals::Result{vals, valsMetadata};
}

struct InputCallVisitor : public InstVisitor<InputCallVisitor> {
  FindIOVals::Result res{};

  void visitCallInst(CallInst &CI) {
    if(auto *F = CI.getCalledFunction()) {
      StringRef name;
      if(isInputFunc(*F, name)) {
        // Append result
        auto newVals = inputValues(CI, name);
        res.ioVals.insert(newVals.ioVals.cbegin(), newVals.ioVals.cend());
        res.ioValsMetadata.insert(res.ioValsMetadata.end(),
                                  newVals.ioValsMetadata.cbegin(),
                                  newVals.ioValsMetadata.cend());

        // // Pretty-print metadata structs for debugging
        // _pretty_print_meta(res.ioValsMetadata);
      }
    }
  }
};

FindIOVals::Result FindIOVals::run(Function &F, FunctionAnalysisManager &FAM) {
  return run(F);
}

FindIOVals::Result FindIOVals::run(Function &F) {
  Result Res = FindIOVals::Result{};
  InputCallVisitor ICV{};

  ICV.visit(&F);
  Res = ICV.res;

  // // Pretty-print metadata structs for debugging
  // _pretty_print_meta(Res.ioValsMetadata);

  // Add argument values to result
  if(F.getName() == "main") {
    for(auto &arg: F.args()) {
      Res.ioVals.insert(&arg);
      Res.ioValsMetadata.push_back({&arg, nullptr, "main"});
    }
  }

  assert(Res.ioVals.size() == Res.ioValsMetadata.size() &&
         "Mismatched value counts");

  return Res;
}

//-----------------------------------------------------------------------------
// FuncReturnIO implementation
//-----------------------------------------------------------------------------

bool usesInputVal(Instruction &I, std::set<Value *> &ioVals) {
  // Track visited instructions to avoid infinite recursion
  static std::set<Instruction *> visited{};

  // LLVM_DEBUG(dbgs() << "Checking ...\n");
  for(Use &U: I.operands()) {
    // Add to visited set
    visited.insert(&I);

    if(auto V = dyn_cast<Value>(U)) {
      // Check if in input values

      if(ioVals.find(V) != ioVals.cend()) {
        // LLVM_DEBUG(dbgs() << "FOUNDIT");
        return true;
      } else if(auto uI = dyn_cast<Instruction>(U)) {

        // Check if visited
        if(visited.find(uI) != visited.cend()) {
          continue;
        }

        // Otherwise, descend
        if(usesInputVal(*uI, ioVals)) {
          return true;
        }
      }
    }
  }

  return false;
}

struct ReturnVisitor : public InstVisitor<ReturnVisitor> {
  std::set<Value *> ioVals;
  FuncReturnIO::Result res{false};

  void visitReturnInst(ReturnInst &RI) {
    // If any return value is dependent on input, set res.returnIsInput to true
    if(auto inst = dyn_cast<Instruction>(&RI)) {
      res.returnIsIO = res.returnIsIO || usesInputVal(*inst, ioVals);
    }
  }
};

FuncReturnIO::Result FuncReturnIO::run(Function &F,
                                       FunctionAnalysisManager &FAM) {
  if(F.getName() == "main") {
    // errs() << "Skipping main function...\n";
    return FuncReturnIO::Result{false};
  }

  auto ioVals = FAM.getCachedResult<FindIOVals>(F)->ioVals;

  // If no IO values, no need to check return values
  if(ioVals.empty()) {
    return FuncReturnIO::Result{false};
  }

  ReturnVisitor RV{.ioVals = ioVals};
  RV.visit(F);

  return RV.res;
}

//-----------------------------------------------------------------------------
// FindIOValsPrinter implementation
//-----------------------------------------------------------------------------
static void printIOVals(raw_ostream &OS,
                        Function &Func,
                        const FindIOVals::Result &findIOValuesResult,
                        const FuncReturnIO::Result &ioFunctionResult) noexcept {

  OS << "IO Values in \"" << Func.getName() << "\":";
  if(findIOValuesResult.ioVals.empty()) {
    OS << " None\n";
    return;
  } else if(ioFunctionResult.returnIsIO) {
    OS << " ** RETURNS IO **\n";
  } else {
    OS << "** DOES NOT RETURN IO **\n";
  }

  // Print instruction for each value in res.ioValsMetadata
  for(const auto &IOVal: findIOValuesResult.ioValsMetadata) {
    OS << "\tValue \"" << IOVal.val->getName() << "\"";

    // Check if inst is null (like arg to main) before trying to print
    if(IOVal.inst) {
      OS << " defined at: " << utils::getInstructionSrc(*IOVal.inst);
    }

    if(IOVal.isFile) {
      OS << " (File descriptor)";
    }

    OS << "\n";
  }
}

PreservedAnalyses FindIOValsPrinter::run(Function &F,
                                         FunctionAnalysisManager &FAM) {
  auto &IOVals = FAM.getResult<FindIOVals>(F);
  auto &IsRetIO = FAM.getResult<FuncReturnIO>(F);
  printIOVals(OS, F, IOVals, IsRetIO);
  return PreservedAnalyses::all();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::AnalysisKey FindIOVals::Key;
llvm::AnalysisKey FuncReturnIO::Key;

PassPluginLibraryInfo getFindIOValsPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION,
          PluginName,
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "FAM.getResult<FindIOVals>(Function)"
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return FindIOVals(); });
                  // #2 REGISTRATION FOR "FAM.getResult<FuncReturnIO>(Function)"
                  FAM.registerPass([&] { return FuncReturnIO(); });
                });
            // #3 REGISTRATION FOR "opt -passes=print<find-io-val>"
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
                    FPM.addPass(FindIOValsPrinter(llvm::outs()));
                    return true;
                  }

                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFindIOValsPluginInfo();
}