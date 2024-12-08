#include "FindIOVals.h"

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
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

using namespace llvm;

static void printIOVals(raw_ostream &OS,
                        Function &Func,
                        const FindIOVals::Result &res) noexcept {

  OS << "IO Values in \"" << Func.getName() << "\":";
  if(res.ioVals.empty()) {
    OS << " None\n";
    return;
  } else {
    OS << "\n";
  }

  // Using a ModuleSlotTracker for printing makes it so full function analysis
  // for slot numbering only occurs once instead of every time an instruction
  // is printed.
  ModuleSlotTracker Tracker(Func.getParent());

  // Print instruction for each value in res.ioValsMetadata
  for(const auto &IOVal: res.ioValsMetadata) {
    OS << "Value " << IOVal.val->getName()
       << " defined at: " << utils::getInstructionSrc(*IOVal.inst) << "\n";
  }

  // for(Value *V: res.ioVals) {
  //   // V->print(OS, Tracker);
  //   if (auto I = dyn_cast<Instruction>(V)) {
  //     utils::printInstructionSrc(OS, *I);
  //   } else {
  //     OS << "Constant Value: ";
  //     V->printAsOperand(OS, false, Func.getParent());
  //   }
  // }
}

// Deprecated? Would like if I could include a description:
// static constexpr char PassName[] = "IO-dependent value locator";

static constexpr char PassArg[] = "find-io-val";
static constexpr char PluginName[] = "FindIOVals";

//------------------------------------------------------------------------------
// FindIOVals implementation
//------------------------------------------------------------------------------

FindIOVals::Result inputValues(CallInst &I, StringRef name) {
  std::vector<Value *> vals{};
  std::vector<IOValMetadata> valsMetadata{};

  if(name == "scanf") {
    // All after first argument
    for(auto arg = I.arg_begin() + 1; arg != I.arg_end(); ++arg) {
      vals.push_back(arg->get());
      valsMetadata.push_back({arg->get(), &I, name.str()});
    }
  } else if(name == "fscanf" || name == "sscanf") {
    // All after second argument
    for(auto arg = I.arg_begin() + 2; arg != I.arg_end(); ++arg) {
      vals.push_back(arg->get());
      valsMetadata.push_back({arg->get(), &I, name.str()});
    }
  } else if(name == "gets" || name == "fgets" || name == "fread") {
    // first argument
    vals.push_back(I.getArgOperand(0));
    valsMetadata.push_back({I.getArgOperand(0), &I, name.str()});
  } else if(name == "getopt") {
    // third argument
    vals.push_back(I.getArgOperand(2));
    valsMetadata.push_back({I.getArgOperand(2), &I, name.str()});
  } else if(name == "fdopen" || name == "freopen" || name == "popen") {
    // Retvals, and mark as file descriptors
    auto retval = dyn_cast_if_present<Value>(&I);
    valsMetadata.push_back({retval, &I, name.str(), true, "File descriptor"});
  } else if(name == "fgetc" || name == "getc" || name == "getc_unlocked" ||
            name == "getchar" || name == "getchar_unlocked" || name == "getw" || name == "getenv") {
    // Retvals, not file descriptors
    auto retval = dyn_cast_if_present<Value>(&I);
    valsMetadata.push_back({retval, &I, name.str()});
  }

  // Add store instructions that use the return value
  for(auto U: I.users()) {
    if(auto SI = dyn_cast<StoreInst>(U)) {
      vals.push_back(SI->getOperand(1));
      valsMetadata.push_back({SI->getOperand(1), SI, name.str()});
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
        res = inputValues(CI, name);
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
  return Res;
}

PreservedAnalyses FindIOValsPrinter::run(Function &F,
                                         FunctionAnalysisManager &FAM) {
  auto &Comparisons = FAM.getResult<FindIOVals>(F);
  printIOVals(OS, F, Comparisons);
  return PreservedAnalyses::all();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::AnalysisKey FindIOVals::Key;

PassPluginLibraryInfo getFindIOValsPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION,
          PluginName,
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "FAM.getResult<FindIOVals>(Function)"
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return FindIOVals(); });
                });
            // #2 REGISTRATION FOR "opt -passes=print<find-io-val>"
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