#include "compiler.h"

void compile(const char* src)
{
    parse(src);
}

InterpretResult interpret(const char* src)
{
    compile(src);
    return INTERPRET_OK;
}