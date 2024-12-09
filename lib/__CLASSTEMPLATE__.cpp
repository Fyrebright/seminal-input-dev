#include "__CLASSTEMPLATE__.h"

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
#define DEBUG_TYPE "__ARGTEMPLATE__"
#endif // DEBUG_TYPE

using namespace llvm;

static constexpr char PassArg[] = "__ARGTEMPLATE__";
static constexpr char PluginName[] = "__CLASSTEMPLATE__";

//------------------------------------------------------------------------------
// __CLASSTEMPLATE__ implementation
//------------------------------------------------------------------------------

__CLASSTEMPLATE__::Result inputValues(CallInst &I, StringRef name) {
  return __CLASSTEMPLATE__::Result{};
}

struct __VISTORTEMPLATE__Visitor
    : public InstVisitor<__VISTORTEMPLATE__Visitor> {
  __CLASSTEMPLATE__::Result res{};

  void visitBranchInst(BranchInst &I) {}
};

__CLASSTEMPLATE__::Result __CLASSTEMPLATE__::run(Function &F,
                                                 FunctionAnalysisManager &FAM) {
  Result Res = __CLASSTEMPLATE__::Result{};
  __VISTORTEMPLATE__Visitor _VISTOR_{};

  _VISTOR_.visit(F);

  return _VISTOR_.res;
}

//------------------------------------------------------------------------------
// __CLASSTEMPLATE__Printer implementation
//------------------------------------------------------------------------------

static void
print__PRINTERTEMPLATE__(raw_ostream &OS,
                         Function &Func,
                         const __CLASSTEMPLATE__::Result &res) noexcept {

  OS << "__DATA__ in \"" << Func.getName() << "\":";
  if(res.__DATA_TEMPLATE__.empty()) {
    OS << " None\n";
    return;
  } else {
    OS << "\n";
  }

  // Print instruction for each value
  for(const auto &I: res.__DATA_TEMPLATE__) {
    OS << "\t" << utils::getInstructionSrc(*I) << "\n";

    OS << "\t\tOperands: ";
    for(Use &U: I->operands()) {
      OS << U->getName() << ", ";
    }
    OS << "\n";
  }
}

PreservedAnalyses __CLASSTEMPLATE__Printer::run(Function &F,
                                                FunctionAnalysisManager &FAM) {
  auto &KeyPoints = FAM.getResult<__CLASSTEMPLATE__>(F);
  print__PRINTERTEMPLATE__(OS, F, KeyPoints);
  return PreservedAnalyses::all();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::AnalysisKey __CLASSTEMPLATE__::Key;

PassPluginLibraryInfo get__CLASSTEMPLATE__PluginInfo() {
  return {LLVM_PLUGIN_API_VERSION,
          PluginName,
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "FAM.getResult<__CLASSTEMPLATE__>(Function)"
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return __CLASSTEMPLATE__(); });
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
                    FPM.addPass(__CLASSTEMPLATE__Printer(llvm::outs()));
                    return true;
                  }

                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return get__CLASSTEMPLATE__PluginInfo();
}