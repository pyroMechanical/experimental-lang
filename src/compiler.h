#ifndef compiler_header
#define compiler_header

#include "core.h"
#include "instruction.h"
#include "parser.h"

bool compile(const char* src, IRBlock* block);


#endif