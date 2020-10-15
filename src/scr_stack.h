
#pragma once
#ifndef SCR_STACK_H
#define SCR_STACK_H

#include "scr_types.h"
#include "scr_heap.h"

#define DEFAULT_STACK_SIZE 1

class Stack
{
	public:
		Stack(void);
		void Init(void);
		void Push(stack_t value);
		stack_t Pop(void);
		stack_t GetRaw(stack_t *pt);
		void SetRaw(stack_t *pt, stack_t value);
		stack_t Peek(int index);
		stack_t* GetBase(void);
		stack_t* GetPointer(void);
		void SetPointer(stack_t *pt);
		void AddPointer(int val);
		bool IsEmpty(void);
	protected:
		stack_t *data;
		stack_t *pointer;
};

#endif