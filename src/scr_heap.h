/*
** 
*/

#pragma once
#ifndef SCR_HEAP_H
#define SCR_HEAP_H

#include "scr_types.h"

#define HEAP_ID_FREE 0

typedef uint16_t heap_id_t;

// Заголовок блока
typedef struct {
	heap_id_t id; // 2 б
	uint16_t len; // 2 б
} heaphdr_t;

class Heap {
	private:
		heap_t *heap;
		uint16_t base;
		uint16_t length;
	public:
		Heap(int _length);
		uint16_t get_base(void);
		heap_t* get_heap(void);
		uint16_t get_length(void);
		void heap_dmp(void);
		heap_id_t alloc(uint16_t size);
		void free(heap_id_t id);
		void realloc(heap_id_t id, uint16_t size);
		uint16_t get_length(heap_id_t id);
		void* get_addr(heap_id_t id);
		uint16_t get_free_size(void);
		heap_t* get_relative_base(void);
		void mem_steal(uint16_t length);
		void mem_unsteal(uint16_t length);
};

#endif
