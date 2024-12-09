#ifndef __412PROJ_UTILS_H
#define __412PROJ_UTILS_H

#include "llvm/IR/Instruction.h"
#include "llvm/Pass.h"

// #ifndef DEBUG_TYPE
// #define DEBUG_TYPE "dev"
// #endif // DEBUG_TYPE

// Forward declarations
namespace llvm {

class Value;
class Function;
class Module;
class raw_ostream;
class Instruction;

} // namespace llvm

namespace utils {

std::string getInstructionSrc(llvm::Instruction &I);

void printInstructionSrc(llvm::raw_ostream &OutS, llvm::Instruction &I);

inline void printInstructionSrc(llvm::raw_ostream &OutS, llvm::Instruction &I) {
  OutS << getInstructionSrc(I) << "\n";
  OutS.flush();
}

unsigned int lineNum(llvm::Instruction &I);

} // namespace utils

#endif // __412PROJ_UTILS_H