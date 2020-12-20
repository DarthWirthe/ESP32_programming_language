/* scr_heap.h
** Виртуальная память
*/

#pragma once
#ifndef SCR_HEAP_H
#define SCR_HEAP_H

#include "scr_types.h"

#define HEAP_ID_FREE 0

// Коды ошибок
#define HEAP_ERR_NOT_FOUND 0x20
#define HEAP_ERR_OUT_OF_MEMORY 0x21
#define HEAP_ERR_CORRUPTED 0x22
#define HEAP_ERR_OVERFLOW 0x23
#define HEAP_ERR_UNDERFLOW 0x24

// Макросы для битовых операций
#define BITREAD(NUM, BIT) (((NUM) >> (BIT)) & 1)
#define BITSET(NUM, BIT) ((NUM) |= (1UL << (BIT)))
#define BITCLEAR(NUM, BIT) ((NUM) &= ~(1UL << (BIT)))
#define BITWRITE(NUM, BIT, VAL) ((VAL) ? BITSET((NUM), (BIT)) : BITCLEAR((NUM), (BIT)))

// Маска для типа блока - 2 старших бита
#define HEAP_TYPE_MASK 0xC0000000U
/* Т.к. ид блока занимает только 14 бит, нужно убирать первые 2*/
#define HEAP_ID_MASK 0x3FFFU

typedef uint8_t  heap_t;
typedef uint16_t u16_t;

// Заголовок блока
typedef struct {
	u16_t id;
	uint16_t len;
} heaphdr_t;

class Heap {
	public:
		Heap(int size);
		void heap_dmp(void);
		u16_t heap_new_id(void);
		bool heap_alloc_internal(u16_t id, uint16_t size);
		u16_t alloc(uint16_t size);
		void free(u16_t id);
		void realloc(u16_t id, uint16_t size);
		uint16_t get_length(u16_t id);
		void* get_addr(u16_t id);
		uint16_t get_free_size(void);
		heap_t* get_relative_base(void);
		void garbage_collect(void);
		void mem_steal(uint16_t length);
		void mem_unsteal(uint16_t length);
		void delete_heap(void);
		// Поля
		heap_t *heap;
		uint16_t base;
		uint16_t length;
};

#endif // SCR_HEAP_H
