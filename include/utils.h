#ifndef UTILS_H
#define UTILS_H

#include "llvm/IR/Instruction.h"
#include "llvm/Pass.h"


// Forward declarations
namespace llvm {

class Value;
class Function;
class Module;
class raw_ostream;

} // namespace llvm

namespace utils {

std::string getInstructionSrc(llvm::Instruction &I);

inline void printInstructionSrc(llvm::raw_ostream &OutS, llvm::Instruction &I) {
  OutS << getInstructionSrc(I) << "\n";
}

} // namespace utils

#endif // UTILS_H