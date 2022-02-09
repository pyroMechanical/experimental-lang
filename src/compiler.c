#include "compiler.h"

bool compile(const char* src, IRBlock* block)
{
    parse(src);

    return true;
}