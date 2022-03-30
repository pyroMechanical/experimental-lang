#ifndef vm_header
#define vm_header

#include "core.h"
#include "instruction.h"
#include "compiler.h"

typedef union
{
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    int16_t i16;
    uint32_t u32;
    int32_t i32;
    uint64_t u64;
    int64_t i64;
    float f32;
    double f64;
    void* ptr;
    uint8_t bytes[8];
    uint16_t segments[4];
} _register;


typedef struct
{
    _register registers[256];
    uint32_t* ip;
    IRBlock* currentBlock;
    uint8_t* stack_begin;
    uint8_t* stack_ptr;
    size_t stack_size;
    size_t stack_capacity;
} Machine;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

InterpretResult interpretSource(const char* src);

InterpretResult runVM(Machine* vm);

#endif