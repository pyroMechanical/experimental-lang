#ifndef instruction_header
#define instruction_header

#include "core.h"

typedef enum {
    OP_HALT, //stops program execution
	OP_PUSH_IP, // push (instruction pointer - chunk beginning) to the stack
	OP_NEW_STACK_FRAME, //push R[FRAME_REGISTER] onto the stack, then store stack.size() in R[FRAME_REGISTER]
		//one-register instructions: 8-bit opcode | 8-bit register A | 16 bits space
	OP_PUSH, //A; push value from R[A] onto stack
	OP_POP,  //A; pop value from stack into R[A]
	OP_RETURN, // pop back of stack and perform absolute jump to it, reset stack size to R[FRAME_REGISTER]
	OP_OUT,  // A, outputs value of R[A] in terminal
		//two-register instructions: 8-bit opcode | 8-bit register A | 8-bit register B | 8 bits space
	OP_MOVE,    // A, B; move value from R[A] to R[B]
	OP_ALLOC,   // A, B; allocate the size of type pointed by R[A] bytes of memory on the heap, store address in R[B]
	OP_CONST_LOW, // A, clears R[A], then writes to bottom two bytes
	OP_CONST_LOW_NEGATIVE, // A, clears R[A] to all ones then overwrites the bottom two bytes
	OP_CONST_MID_LOW,  // A, writes to second lowest two bytes
	OP_CONST_MID_HIGH, // A, writes to second higest two bytes
	OP_CONST_HIGH, // A, writes to highest two bytes
	OP_MOVE_FROM_STACK_FRAME, // A, B; R[B] = stack[R[FRAME_REGISTER] + R[A]]
	OP_STORE_8,  // A, B, C; store value from R[A] in memory address R[B]
	OP_STORE_16,  // A, B, C; store value from R[A] in memory address R[B]
	OP_STORE_32,  // A, B, C; store value from R[A] in memory address R[B]
	OP_STORE_64,  // A, B, C; store value from R[A] in memory address R[B]
	OP_LOAD_8,  // A, B, C; load value from address R[B] to R[A]
	OP_LOAD_16,  // A, B, C; load value from address R[B] to R[A]
	OP_LOAD_32,  // A, B, C; load value from address R[B] to R[A]
	OP_LOAD_64,  // A, B, C; load value from address R[B] to R[A]
		//three-register instructions: 8-bit opcode | 8-bit register A | 8-bit register B | 8-bit register C
	OP_ALLOC_ARRAY, //A, B, C; allocate array with length R[A] and span R[B] and store pointer in R[C]
		//signed integers are sign-extended, so one addition/subtraction operation works for both
	OP_INT_ADD, //A, B, C; R[C] = R[A] + R[B]
	OP_INT_SUB, //A, B, C; R[C] = R[A] - R[B]
	OP_INT_NEGATE, //A, B; R[B] = -R[A]
		//arithmetic and comparison operations on unsigned integers
	OP_UNSIGN_MUL, //A, B, C; R[C] = R[A] * R[B]
	OP_UNSIGN_DIV, //A, B, C; R[C] = R[A] / R[B]
	OP_BIT_SHIFT_RIGHT, //A, B, C; R[C] = R[A] >> R[B]
	OP_BIT_SHIFT_LEFT,//A, B, C; R[C] = R[A] << R[B]
		//arithmetic and comparison operations on unsigned integers
	OP_SIGN_MUL, //A, B, C; R[C] = R[A] * R[B]
	OP_SIGN_DIV, //A, B, C; R[C] = R[A] / R[B]
		//arithmetic and comparison operations on single-precision floats
	OP_FLOAT_ADD, //A, B, C; R[C] = R[A] + R[B]
	OP_FLOAT_SUB, //A, B, C; R[C] = R[A] - R[B]
	OP_FLOAT_MUL, //A, B, C; R[C] = R[A] * R[B]
	OP_FLOAT_DIV, //A, B, C; R[C] = R[A] / R[B]
	OP_FLOAT_NEGATE, //A, B; R[B] = -R[A]
		//arithmetic and comparison operators on double-precision floats
	OP_DOUBLE_ADD, //A, B, C; R[C] = R[A] + R[B]
	OP_DOUBLE_SUB, //A, B, C; R[C] = R[A] - R[B]
	OP_DOUBLE_MUL, //A, B, C; R[C] = R[A] * R[B]
	OP_DOUBLE_DIV, //A, B, C; R[C] = R[A] / R[B]
	OP_DOUBLE_NEGATE, //A, B; R[B] = -R[A]
		//comparison operations set the value of the comparison register with their output value
	OP_UNSIGN_LESS, //A, B, C; R[C] = R[A] < R[B]
	OP_UNSIGN_GREATER, //A, B, C; R[C] = R[A] > R[B]
	OP_SIGN_LESS, //A, B, C; R[C] = R[A] < R[B]
	OP_SIGN_GREATER, //A, B, C; R[C] = R[A] > R[B]
	OP_INT_EQUAL, //A, B, C; R[C] = R[A] == R[B]
	OP_FLOAT_LESS, //A, B, C; R[C] = R[A] < R[B]
	OP_FLOAT_GREATER, //A, B, C; R[C] = R[A] > R[B]
	OP_FLOAT_EQUAL, //A, B, C; R[C] = R[A] == R[B]
	OP_DOUBLE_LESS, //A, B, C; R[C] = R[A] < R[B]
	OP_DOUBLE_GREATER, //A, B, C; R[C] = R[A] > R[B]
	OP_DOUBLE_EQUAL, //A, B, C; R[C] = R[A] == R[B]
		//conversion operations
	OP_INT_TO_FLOAT, // A, B; R[B] = (float)R[A]
	OP_FLOAT_TO_INT, // A, B; R[B] = (int)R[A]
	OP_FLOAT_TO_DOUBLE, // A, B; R[B] = (double)R[A]
	OP_DOUBLE_TO_FLOAT, // A, B; R[B] = (float)R[A]
	OP_INT_TO_DOUBLE, // A, B; R[B] = (double)R[A]
	OP_DOUBLE_TO_INT, // A, B; R[B] = (int)R[A]
	OP_BITWISE_AND, //A, B, C; R[C] = R[A] & R[B]
	OP_BITWISE_OR, //A, B, C; R[C] = R[A] | R[B]
	OP_LOGICAL_AND, //A, B, C; R[C] = R[A] && R[B]
	OP_LOGICAL_OR, //A, B, C; R[C] = R[A] || R[B]
	OP_LOGICAL_NOT, //A, B; R[B] = !R[A]
		//jump instructions
		//relative jump instruction: 8-bit opcode | 24-bit signed integer offset
	OP_RELATIVE_JUMP, // instruction pointer += signed integer offset
	OP_RELATIVE_JUMP_IF_TRUE, // if(comparison register), instruction pointer += signed integer offset
			//absolute jump instruction: 8-bit opcode | 8-bit register A | 16 bits space
	OP_REGISTER_JUMP, // instruction pointer = chunk beginning + R[A] 
	OP_REGISTER_JUMP_IF_TRUE, // if(comparison register) instruciton pointer = chunk beginning + R[A]
} IR;

typedef struct {
    uint32_t* instructions;
    size_t count;
    size_t capacity;
} IRBlock;

void initBlock(IRBlock* block);

void freeBlock(IRBlock* block);

void writeBlock(IRBlock* block, uint32_t instruction);

uint32_t instructionFromA(uint8_t op, uint8_t A);
uint32_t instructionFromAB(uint8_t op, uint8_t A, uint8_t B);
uint32_t instructionFromABC(uint8_t op, uint8_t A, uint8_t B, uint8_t C);

#endif