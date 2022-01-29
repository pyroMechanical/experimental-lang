#ifndef compiler_header
#define compiler_header

#include "core.h"
#include "parser.h"

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void compile(const char* src);
InterpretResult interpret(const char* src);

#endif