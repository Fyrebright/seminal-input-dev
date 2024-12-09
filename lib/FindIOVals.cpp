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

  if(name == "scanf") {
    // All after first argument
    for(auto arg = I.arg_begin() + 1; arg != I.arg_end(); ++arg) {
      vals.insert(arg->get());
      valsMetadata.push_back({arg->get(), &I, name.str()});
    }
  } else if(name == "fscanf" || name == "sscanf") {
    // All after second argument
    for(auto arg = I.arg_begin() + 2; arg != I.arg_end(); ++arg) {
      vals.insert(arg->get());
      valsMetadata.push_back({arg->get(), &I, name.str()});
    }
  } else if(name == "gets" || name == "fgets" || name == "fread") {
    // first argument
    vals.insert(I.getArgOperand(0));
    valsMetadata.push_back({I.getArgOperand(0), &I, name.str()});
  } else if(name == "getopt") {
    // third argument
    vals.insert(I.getArgOperand(2));
    valsMetadata.push_back({I.getArgOperand(2), &I, name.str()});
  } else if(name == "fdopen" || name == "freopen" || name == "popen") {
    // Retvals, and mark as file descriptors
    auto retval = dyn_cast_if_present<Value>(&I);
    vals.insert(retval);
    valsMetadata.push_back({retval, &I, name.str(), true, "File descriptor"});
  } else if(name == "fgetc" || name == "getc" || name == "getc_unlocked" ||
            name == "getchar" || name == "getchar_unlocked" || name == "getw" ||
            name == "getenv") {
    // Retvals, not file descriptors
    auto retval = dyn_cast_if_present<Value>(&I);
    vals.insert(retval);
    valsMetadata.push_back({retval, &I, name.str()});
  }

  // Add store instructions that use the return value
  for(auto U: I.users()) {
    if(auto SI = dyn_cast<StoreInst>(U)) {
      vals.insert(SI->getOperand(1));
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

//-----------------------------------------------------------------------------
// FuncReturnIO implementation
//-----------------------------------------------------------------------------

bool usesInputVal(Instruction &I, std::set<Value *> &ioVals) {
  // errs() << "Checking ...\n";
  for(Use &U: I.operands()) {
    // errs() << "User " << U.get()->getName() << "\n";
    if (auto instU = dyn_cast<Instruction>(U)) {
        // utils::printInstructionSrc(errs(), *instU);
    }

    if(auto V = dyn_cast<Value>(U)) {
      // Check if in input values
      if(ioVals.find(V) != ioVals.cend()) {
        // errs() << "FOUNDIT";
        return true;
      } else if(auto uI = dyn_cast<Instruction>(U)) {
        // errs() << "sadge:";
        // utils::printInstructionSrc(errs(), *uI);
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
    res.returnIsInput =
        res.returnIsInput || usesInputVal(cast<Instruction>(RI), ioVals);
  }
};

FuncReturnIO::Result FuncReturnIO::run(Function &F,
                                       FunctionAnalysisManager &FAM) {
  if(F.getName() == "main") {
    errs() << "Skipping main function...\n";
    return FuncReturnIO::Result{false};
  }

  auto ioVals = FAM.getCachedResult<FindIOVals>(F)->ioVals;

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
  } else if(ioFunctionResult.returnIsInput) {
    OS << " ** RETURNS IO **\n";
  } else {
    OS << "** DOES NOT RETURN IO **\n";
  }

  // Using a ModuleSlotTracker for printing makes it so full function analysis
  // for slot numbering only occurs once instead of every time an instruction
  // is printed.
  ModuleSlotTracker Tracker(Func.getParent());

  // Print instruction for each value in res.ioValsMetadata
  for(const auto &IOVal: findIOValuesResult.ioValsMetadata) {
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