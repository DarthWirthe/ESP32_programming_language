
#include "scr_stack.h"

// Стек на данный момент 32-битный
// 

Stack::Stack(){}

void Stack::Init()
{
	data = (stack_t*)heap_get_base();
	pointer = data - 1;
	heap_steal(DEFAULT_STACK_SIZE * sizeof(stack_t)); // память берётся из кучи
}

void Stack::Push(stack_t value)
{
	*(++pointer) = value;
}

stack_t Stack::Pop()
{
	return *(pointer--);
}

stack_t Stack::GetRaw(stack_t *pt)
{
	return *pt;
}

void Stack::SetRaw(stack_t *pt, stack_t value)
{
	*pt = value;
}

stack_t Stack::Peek(int index)
{
	return pointer[-index];
}

stack_t* Stack::GetBase()
{
	return data;
}

stack_t* Stack::GetPointer()
{
	return pointer;
}

void Stack::SetPointer(stack_t *pt)
{
	pointer = pt;
}

void Stack::AddPointer(int val)
{
	pointer += val;
}

bool Stack::IsEmpty() {
	if (pointer < data)
		return true;
	return false;
}



