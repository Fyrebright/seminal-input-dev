
#include "utils.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/FormatVariadic.h"

#include <filesystem>
#include <fstream>
#include <iostream>

// Forward declarations
namespace llvm {

class Instruction;
class raw_ostream;

} // namespace llvm

using namespace llvm;
namespace utils {

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
    return -1;
}

/**
 * @brief Returns a string with the source code line corresponding to the given
 * instruction.
 *
 * @param I The instruction for which to retrieve the source code line.
 * @return std::string with line number, source code line, and instruction type.
 */
std::string getInstructionSrc(Instruction &I) {
  if(const DILocation *Loc = I.getDebugLoc()) {
    unsigned lineNumber = Loc->getLine();
    StringRef File = Loc->getFilename();
    StringRef Dir = Loc->getDirectory();
    // bool ImplicitCode = Loc->isImplicitCode();

    try {
      std::ifstream srcFile(
          std::filesystem::canonical((Dir + "/" + File).str()), std::ios::in);
      if(srcFile.good()) {
        GotoLine(srcFile, lineNumber);
      } else {
        return "Error: Unable to read source file";
      }
      GotoLine(srcFile, lineNumber);
      std::string line;
      getline(srcFile, line);

      // construct a string with the instruction name and the source code line
      std::string instructionSrc =
          formatv("{0} : {1} ({2})", lineNumber, line, I.getOpcodeName());

      srcFile.close();

      return instructionSrc;
    } catch(const std::filesystem::filesystem_error &e) {
      return "Error accessing file: " + std::string(e.what());
    }
  }
  return formatv(
      "{0} : {1} ({2})", "No debug info", I.getName(), I.getOpcodeName());
}

// sed -E 's/(\w+Inst)/void visit$1($1 &I) {errs() << "$1(" << utils::lineNum(I) << ")\\n";}/gm;t'
struct AllVisitor : public InstVisitor<AllVisitor> {
  void visitAddrSpaceCastInst(AddrSpaceCastInst &I) {errs() << "AddrSpaceCastInst(" << utils::lineNum(I) << ")\n";}
  void visitAllocaInst(AllocaInst &I) {errs() << "AllocaInst(" << utils::lineNum(I) << ")\n";}
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I) {errs() << "AtomicCmpXchgInst(" << utils::lineNum(I) << ")\n";}
  void visitAtomicRMWInst(AtomicRMWInst &I) {errs() << "AtomicRMWInst(" << utils::lineNum(I) << ")\n";}
  void visitBitCastInst(BitCastInst &I) {errs() << "BitCastInst(" << utils::lineNum(I) << ")\n";}
  void visitBranchInst(BranchInst &I) {errs() << "BranchInst(" << utils::lineNum(I) << ")\n";}
  void visitCallBrInst(CallBrInst &I) {errs() << "CallBrInst(" << utils::lineNum(I) << ")\n";}
  void visitCallInst(CallInst &I) {errs() << "CallInst(" << utils::lineNum(I) << ")\n";}
  void visitCastInst(CastInst &I) {errs() << "CastInst(" << utils::lineNum(I) << ")\n";}
  void visitCatchPadInst(CatchPadInst &I) {errs() << "CatchPadInst(" << utils::lineNum(I) << ")\n";}
  void visitCatchReturnInst(CatchReturnInst &I) {errs() << "CatchReturnInst(" << utils::lineNum(I) << ")\n";}
  void visitCatchSwitchInst(CatchSwitchInst &I) {errs() << "CatchSwitchInst(" << utils::lineNum(I) << ")\n";}
  void visitCleanupPadInst(CleanupPadInst &I) {errs() << "CleanupPadInst(" << utils::lineNum(I) << ")\n";}
  void visitCleanupReturnInst(CleanupReturnInst &I) {errs() << "CleanupReturnInst(" << utils::lineNum(I) << ")\n";}
  void visitCmpInst(CmpInst &I) {errs() << "CmpInst(" << utils::lineNum(I) << ")\n";}
  void visitExtractElementInst(ExtractElementInst &I) {errs() << "ExtractElementInst(" << utils::lineNum(I) << ")\n";}
  void visitExtractValueInst(ExtractValueInst &I) {errs() << "ExtractValueInst(" << utils::lineNum(I) << ")\n";}
  void visitFCmpInst(FCmpInst &I) {errs() << "FCmpInst(" << utils::lineNum(I) << ")\n";}
  void visitFenceInst(FenceInst &I) {errs() << "FenceInst(" << utils::lineNum(I) << ")\n";}
  void visitFPExtInst(FPExtInst &I) {errs() << "FPExtInst(" << utils::lineNum(I) << ")\n";}
  void visitFPToSIInst(FPToSIInst &I) {errs() << "FPToSIInst(" << utils::lineNum(I) << ")\n";}
  void visitFPToUIInst(FPToUIInst &I) {errs() << "FPToUIInst(" << utils::lineNum(I) << ")\n";}
  void visitFPTruncInst(FPTruncInst &I) {errs() << "FPTruncInst(" << utils::lineNum(I) << ")\n";}
  void visitFreezeInst(FreezeInst &I) {errs() << "FreezeInst(" << utils::lineNum(I) << ")\n";}
  void visitFuncletPadInst(FuncletPadInst &I) {errs() << "FuncletPadInst(" << utils::lineNum(I) << ")\n";}
  void visitGetElementPtrInst(GetElementPtrInst &I) {errs() << "GetElementPtrInst(" << utils::lineNum(I) << ")\n";}
  void visitICmpInst(ICmpInst &I) {errs() << "ICmpInst(" << utils::lineNum(I) << ")\n";}
  void visitIndirectBrInst(IndirectBrInst &I) {errs() << "IndirectBrInst(" << utils::lineNum(I) << ")\n";}
  void visitInsertElementInst(InsertElementInst &I) {errs() << "InsertElementInst(" << utils::lineNum(I) << ")\n";}
  void visitInsertValueInst(InsertValueInst &I) {errs() << "InsertValueInst(" << utils::lineNum(I) << ")\n";}
  void visitIntToPtrInst(IntToPtrInst &I) {errs() << "IntToPtrInst(" << utils::lineNum(I) << ")\n";}
  void visitInvokeInst(InvokeInst &I) {errs() << "InvokeInst(" << utils::lineNum(I) << ")\n";}
  void visitLandingPadInst(LandingPadInst &I) {errs() << "LandingPadInst(" << utils::lineNum(I) << ")\n";}
  void visitLoadInst(LoadInst &I) {errs() << "LoadInst(" << utils::lineNum(I) << ")\n";}
  void visitPtrToIntInst(PtrToIntInst &I) {errs() << "PtrToIntInst(" << utils::lineNum(I) << ")\n";}
  void visitResumeInst(ResumeInst &I) {errs() << "ResumeInst(" << utils::lineNum(I) << ")\n";}
  void visitReturnInst(ReturnInst &I) {errs() << "ReturnInst(" << utils::lineNum(I) << ")\n";}
  void visitSelectInst(SelectInst &I) {errs() << "SelectInst(" << utils::lineNum(I) << ")\n";}
  void visitSExtInst(SExtInst &I) {errs() << "SExtInst(" << utils::lineNum(I) << ")\n";}
  void visitShuffleVectorInst(ShuffleVectorInst &I) {errs() << "ShuffleVectorInst(" << utils::lineNum(I) << ")\n";}
  void visitSIToFPInst(SIToFPInst &I) {errs() << "SIToFPInst(" << utils::lineNum(I) << ")\n";}
  void visitStoreInst(StoreInst &I) {errs() << "StoreInst(" << utils::lineNum(I) << ")\n";}
  void visitSwitchInst(SwitchInst &I) {errs() << "SwitchInst(" << utils::lineNum(I) << ")\n";}
  void visitTruncInst(TruncInst &I) {errs() << "TruncInst(" << utils::lineNum(I) << ")\n";}
  void visitUIToFPInst(UIToFPInst &I) {errs() << "UIToFPInst(" << utils::lineNum(I) << ")\n";}
  void visitUnaryInstruction(UnaryInstruction &I) {errs() << "UnaryInstruction(" << utils::lineNum(I) << ")\n";}
  void visitUnreachableInst(UnreachableInst &I) {errs() << "UnreachableInst(" << utils::lineNum(I) << ")\n";}
  void visitVAArgInst(VAArgInst &I) {errs() << "VAArgInst(" << utils::lineNum(I) << ")\n";}
  void visitZExtInst(ZExtInst &I) {errs() << "ZExtInst(" << utils::lineNum(I) << ")\n";}
};

} // namespace utils