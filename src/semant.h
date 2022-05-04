#ifndef semant_header
#define semant_header

#include "parser.h"

std::shared_ptr<ProgramNode> analyze(const char* src);

#endif