#include "compiler.h"
#include "semant.h"

bool compile(const char* src)
{
    ProgramNode* ast = analyze(src);
    if(ast == NULL) return false;
    else return true;
}