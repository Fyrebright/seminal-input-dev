
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
  return "No debug info";
}

} // namespace utils