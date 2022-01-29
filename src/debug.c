#include <stdio.h>
#include <stdlib.h>
#include "debug.h"

void disassembleBlock(IRBlock* block, const char* name)
{
	printf_s("== %s ==\n", name);
	for(size_t offset = 0; offset < block->count;)
	{
		offset = disassembleInstruction(block, offset);
	}
}

size_t disassembleInstruction(IRBlock* block, size_t offset)
{
    printf_s("%04d ", offset);
    if(offset > block->count) exit(2);
    uint8_t instruction = (uint8_t)(block->instructions[offset] >> 24);
	switch (instruction)
	{
		case OP_HALT: return SimpleInstruction(block, "OP_HALT", offset);
		case OP_PUSH: return AInstruction(block, "OP_PUSH", offset);
		case OP_POP: return AInstruction(block, "OP_POP", offset);
		case OP_RETURN: return SimpleInstruction(block, "OP_RETURN", offset);
		case OP_OUT: return AInstruction(block, "OP_OUT", offset);
		case OP_PUSH_IP: return AInstruction(block, "OP_PUSH_IP", offset);
		case OP_MOVE: return ABInstruction(block, "OP_MOVE", offset);
		case OP_ALLOC: return ABInstruction(block, "OP_ALLOC", offset);
		case OP_CONST_LOW: return ConstantInstruction(block, "OP_CONST_LOW", offset);
		case OP_CONST_LOW_NEGATIVE: return ConstantInstruction(block, "OP_CONST_LOW_NEGATIVE", offset);
		case OP_CONST_MID_LOW:  return ConstantInstruction(block, "OP_CONST_MID_LOW", offset);
		case OP_CONST_MID_HIGH: return ConstantInstruction(block, "OP_CONST_MID_HIGH", offset);
		case OP_CONST_HIGH: return ConstantInstruction(block, "OP_CONST_HIGH", offset);
		case OP_STORE_8:  return ABInstruction(block, "OP_STORE_8", offset);
		case OP_STORE_16:  return ABInstruction(block, "OP_STORE_16", offset);
		case OP_STORE_32:  return ABInstruction(block, "OP_STORE_32", offset);
		case OP_STORE_64:  return ABInstruction(block, "OP_STORE_64", offset);
		case OP_LOAD_8:  return ABInstruction(block, "OP_LOAD_8", offset);
		case OP_LOAD_16:  return ABInstruction(block, "OP_LOAD_16", offset);
		case OP_LOAD_32:  return ABInstruction(block, "OP_LOAD_32", offset);
		case OP_LOAD_64:  return ABInstruction(block, "OP_LOAD_64", offset);
		case OP_ALLOC_ARRAY: return ABCInstruction(block, "OP_ALLOC_ARRAY", offset);
		case OP_INT_ADD: return ABCInstruction(block, "OP_INT_ADD", offset);
		case OP_INT_SUB: return ABCInstruction(block, "OP_INT_SUB", offset);
		case OP_INT_NEGATE: return ABInstruction(block, "OP_INT_NEGATE", offset);
		case OP_UNSIGN_MUL: return ABCInstruction(block, "OP_USIGN_MUL", offset);
		case OP_UNSIGN_DIV: return ABCInstruction(block, "OP_USIGN_DIV", offset);
		case OP_SIGN_MUL: return ABCInstruction(block, "OP_SIGN_MUL", offset);
		case OP_SIGN_DIV: return ABCInstruction(block, "OP_SIGN_DIV", offset);
		case OP_FLOAT_ADD: return ABCInstruction(block, "OP_FLOAT_ADD", offset);
		case OP_FLOAT_SUB: return ABCInstruction(block, "OP_FLOAT_SUB", offset);
		case OP_FLOAT_MUL: return ABCInstruction(block, "OP_FLOAT_MUL", offset);
		case OP_FLOAT_DIV: return ABCInstruction(block, "OP_FLOAT_DIV", offset);
		case OP_FLOAT_NEGATE: return ABInstruction(block, "OP_FLOAT_NEGATE", offset);
		case OP_DOUBLE_ADD: return ABCInstruction(block, "OP_DOUBLE_ADD", offset);
		case OP_DOUBLE_SUB: return ABCInstruction(block, "OP_DOUBLE_SUB", offset);
		case OP_DOUBLE_MUL: return ABCInstruction(block, "OP_DOUBLE_MUL", offset);
		case OP_DOUBLE_DIV: return ABCInstruction(block, "OP_DOUBLE_DIV", offset);
		case OP_DOUBLE_NEGATE: return ABInstruction(block, "OP_DOUBLE_NEGATE", offset);
		case OP_UNSIGN_LESS:    return ABCInstruction(block, "OP_USIGN_LESS", offset);
		case OP_UNSIGN_GREATER: return ABCInstruction(block, "OP_USIGN_LESS", offset);
		case OP_SIGN_LESS: 	    return ABCInstruction(block, "OP_SIGN_LESS", offset);
		case OP_SIGN_GREATER:   return ABCInstruction(block, "OP_SIGN_LESS", offset);
		case OP_INT_EQUAL:      return ABCInstruction(block, "OP_INT_EQUAL", offset);
		case OP_FLOAT_LESS:    return ABCInstruction(block, "OP_FLOAT_LESS", offset);
		case OP_FLOAT_GREATER: return ABCInstruction(block, "OP_FLOAT_GREATER", offset);
		case OP_FLOAT_EQUAL:   return ABCInstruction(block, "OP_FLOAT_EQUAL", offset);
		case OP_DOUBLE_LESS:    return ABCInstruction(block, "OP_DOUBLE_LESS", offset);
		case OP_DOUBLE_GREATER:	return ABCInstruction(block, "OP_DOUBLE_GREATER", offset);
		case OP_DOUBLE_EQUAL:	return ABCInstruction(block, "OP_DOUBLE_EQUAL", offset);
		case OP_INT_TO_FLOAT:   return ABInstruction(block, "OP_INT_TO_FLOAT", offset);
		case OP_FLOAT_TO_INT:   return ABInstruction(block, "OP_FLOAT_TO_INT", offset);
		case OP_FLOAT_TO_DOUBLE:  return ABInstruction(block, "OP_FLOAT_TO_DOUBLE", offset);
		case OP_DOUBLE_TO_FLOAT:  return ABInstruction(block, "OP_DOUBLE_TO_FLOAT", offset);
		case OP_INT_TO_DOUBLE:   return ABInstruction(block, "OP_INT_TO_DOUBLE", offset);
		case OP_DOUBLE_TO_INT:   return ABInstruction(block, "OP_DOUBLE_TO_INT", offset);
		case OP_BITWISE_AND: return ABCInstruction(block, "OP_BITWISE_AND", offset);
		case OP_BITWISE_OR:  return ABCInstruction(block, "OP_BITWISE_OR", offset);
		case OP_LOGICAL_AND: return ABCInstruction(block, "OP_LOGICAL_AND", offset);
		case OP_LOGICAL_OR:  return ABCInstruction(block, "OP_LOGICAL_OR", offset);
		case OP_LOGICAL_NOT: return ABInstruction(block, "OP_LOGICAL_NOT", offset);
		case OP_RELATIVE_JUMP: return JumpInstruction(block, "OP_RELATIVE_JUMP", offset);
		case OP_RELATIVE_JUMP_IF_TRUE: return JumpInstruction(block, "OP_RELATIVE_JUMP_IF_TRUE", offset);
		case OP_REGISTER_JUMP: return ABInstruction(block, "OP_REGISTER_JUMP", offset);
		case OP_REGISTER_JUMP_IF_TRUE: return ABInstruction(block, "OP_REGISTER_JUMP_IF_TRUE", offset);
	}
}

size_t SimpleInstruction(IRBlock* block, const char* name, size_t offset)
{
    printf_s("%s\n", name);
    return offset + 1;
}

size_t AInstruction(IRBlock* block, const char* name, size_t offset)
{
    if(offset > block->count) exit(2);
    uint8_t A = (uint8_t)(block->instructions[offset] >> 16);
    printf("%s %03d\n", name, A);
    return offset + 1; 
}

size_t ABInstruction(IRBlock* block, const char* name, size_t offset)
{
    if(offset > block->count) exit(2);
    uint8_t A = (uint8_t)(block->instructions[offset] >> 16);
    uint8_t B = (uint8_t)(block->instructions[offset] >> 8);
    printf_s("%s %03d %03d\n", name, A, B);
    return offset + 1; 
}

size_t ABCInstruction(IRBlock* block, const char* name, size_t offset)
{
    if(offset > block->count) exit(2);
    uint8_t A = (uint8_t)(block->instructions[offset] >> 16);
    uint8_t B = (uint8_t)(block->instructions[offset] >> 8);
    uint8_t C = (uint8_t)(block->instructions[offset]);
    printf_s("%s %03d %03d %03d\n", name, A, B, C);
    return offset + 1; 
}

size_t ConstantInstruction(IRBlock* block, const char* name, size_t offset)
{
    if(offset > block->count) exit(2);
    uint8_t A = (uint8_t)(block->instructions[offset] >> 16);
    uint16_t constant = (uint16_t)block->instructions[offset];
    printf_s("%s %03d %d\n", name, A, constant);
    return offset + 1; 
}

size_t JumpInstruction(IRBlock* block, const char* name, size_t offset)
{
    if(offset > block->count) exit(2);
    uint32_t jumpSize = block->instructions[offset];
    jumpSize += (0xFF000000 * (jumpSize >> 23));
    printf_s("%s %d\n", name, jumpSize);
    return offset + 1; 
}