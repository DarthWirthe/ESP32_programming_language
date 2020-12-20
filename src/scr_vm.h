
/**
*** Заголовочный файл.
*** Виртуальная машина.
**/

#pragma once
#ifndef SCR_VM_H
#define SCR_VM_H

#include "scr_types.h"
#include "scr_heap.h"
#include "scr_opcodes.h"

#define IMMEDIATE_MASK_16 0x8000 // для знаковых типов 16 бит
#define IMMEDIATE_MASK_32 0x80000000 // для знаковых типов 32 бит

#define VM_NOERROR 0x00
#define VM_ERR_UNKNOWN_INSTR 0x01
#define VM_ERR_EXPECTED_FUNCTION 0x08
#define VM_ERR_INDEX_OUT_OF_RANGE 0x09

typedef int8_t  char_t;
typedef int16_t short_t;
typedef int32_t int_t;

typedef uint32_t stack_t;

typedef union {
    char  b[4];
    short s[2];
    int   i[1];
    float f[1];
} union_t;

typedef union {
    uint16_t U16;
    uint16_t I16;
    struct {
        uint8_t L, H;
    } U8;
    int_t I32;
} arg_t;

#define FRAME_REQUIREMENTS 4

class VM
{
	public:
		VM(void);
		VM(uint16_t h_size, uint8_t *ptcode, num_t *nums, char *strs, int g_count);
        stack_t LoadString(char *c);
		int Run(void);
        void StackPush(stack_t val);
        stack_t StackPop(void);
        void CleanMemory(void);
	private:
		Heap *heap;
		uint8_t *code_base; // указатель на начало кода
		uint8_t *stack_base; // указатель на начало стека
		num_t *consts_num_base;
        char *consts_str_base;
        stack_t *_sp;
        int globals_count;
};

#endif // SCR_VM_H
