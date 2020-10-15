
/*
	Виртуальная машина, заголовочный файл.
*/

#pragma once
#ifndef SCR_VM_H
#define SCR_VM_H

#include "scr_parser.h"
#include "scr_tref.h"
#include "scr_types.h"
#include "scr_heap.h"
//#include "scr_stack.h"
#include "scr_opcodes.h"
//#include "utils/scr_dbg.h"

#define FRAME_REQUIREMENTS 3

stack_t float_to_stack(float val);
float stack_to_float(stack_t val);
int_t uint_to_int32(uint32_t val);
short_t uint_to_short16(uint16_t val);

class VM
{
	public:
		VM(Parser obj);
		void Execute(void);
		void Assembly(void);
		void Interpret(void);
		Parser _parser_obj;
		
		uint8_t *code_base;
		uint8_t *stack_base;
		num_t *consts;
};

#endif