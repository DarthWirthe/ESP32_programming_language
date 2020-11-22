
/*
	Заголовочный файл.
	Виртуальная машина.
*/

#pragma once
#ifndef SCR_VM_H
#define SCR_VM_H

#include "scr_types.h"
#include "scr_heap.h"
#include "scr_opcodes.h"

#define FRAME_REQUIREMENTS 3

class VM
{
	public:
		VM(void);
		VM(uint16_t h_size, uint8_t *code, num_t *c);
		void Run(void);
	private:
		Heap *heap;
		uint8_t *code_base;
		uint8_t *stack_base;
		num_t *consts;
};

#endif
