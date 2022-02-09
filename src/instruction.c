#include <stdlib.h>

#include "instruction.h"
#include "memory.h"

void initBlock(IRBlock* block)
{
    block->count = block->capacity = 0;
    block->instructions = NULL;
}

void freeBlock(IRBlock* block)
{
    FREE_ARRAY(uint32_t, block->instructions, block->capacity);
    initBlock(block);
}

void writeBlock(IRBlock* block, uint32_t instruction)
{
    if(block->capacity < block->count + 1)
    {
        size_t oldCapacity = block->capacity;
        block->capacity = GROW_CAPACITY(oldCapacity);
        block->instructions = GROW_ARRAY(uint32_t, block->instructions, oldCapacity, block->capacity);
    }

    block->instructions[block->count] = instruction;
    block->count++;
}

uint32_t instructionFromA(uint8_t op, uint8_t A)
{
    uint32_t result = 0;
    result += op;
    result = (result << 8) + A;
    result = result << 16;
    return result;
}

uint32_t instructionFromAB(uint8_t op, uint8_t A, uint8_t B)
{
    uint32_t result = 0;
    result += op;
    result = (result << 8) + A;
    result = (result << 8) + B;
    result = result << 8;
    return result;
}

uint32_t instructionFromABC(uint8_t op, uint8_t A, uint8_t B, uint8_t C)
{
    uint32_t result = 0;
    result += op;
    result = (result << 8) + A;
    result = (result << 8) + B;
    result = (result << 8) + C;
    return result;
}