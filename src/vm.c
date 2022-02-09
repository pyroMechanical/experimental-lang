#include "vm.h"
#include "compiler.h"

InterpretResult interpret(const char* src)
{
    IRBlock block;
    initBlock(&block);

    if(!compile(src, &block))
    {
        freeBlock(&block);
        return INTERPRET_COMPILE_ERROR;
    }
    Machine vm;
    vm.currentBlock = &block;
    vm.ip = vm.currentBlock->instructions;
    
    InterpretResult result = run(&vm);

    freeBlock(&block);

    return result;
}