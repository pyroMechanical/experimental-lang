#include "vm.h"
#include "compiler.h"

InterpretResult interpretSource(const char* src)
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
    
    InterpretResult result = runVM(&vm);

    freeBlock(&block);

    return result;
}

InterpretResult runVM(Machine* vm)
{
    return INTERPRET_OK;
}