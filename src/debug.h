#ifndef debug_header
#define debug_header

#include "core.h"
#include "instruction.h"

void disassembleBlock(IRBlock* block, const char* name);

size_t disassembleInstruction(IRBlock* block, size_t offset);
size_t SimpleInstruction(IRBlock* block, const char* name, size_t offset);
size_t ConstantInstruction(IRBlock* block, const char* name, size_t offset);
size_t AInstruction(IRBlock* block, const char* name, size_t offset);
size_t ABInstruction(IRBlock* block, const char* name, size_t offset);
size_t ABCInstruction(IRBlock* block, const char* name, size_t offset);
size_t JumpInstruction(IRBlock* block, const char* name, size_t offset);


#endif