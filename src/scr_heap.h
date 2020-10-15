
#pragma once
#ifndef SCR_HEAP_H
#define SCR_HEAP_H

#include <stdint.h>

#define DEFAULT_HEAP_SIZE 256
#define HEAP_ID_FREE 0

typedef uint16_t heap_id_t;

void heap_dbg(void);
heap_id_t heap_alloc(uint16_t size);
void heap_free(heap_id_t id);
void heap_realloc(heap_id_t id, uint16_t size);
uint16_t heap_get_length(heap_id_t id);
void* heap_get_addr(heap_id_t id);
void heap_init(void);
uint16_t heap_get_free_size(void);
uint8_t* heap_get_base(void);
uint8_t* heap_get_relative_base(void);
void heap_steal(uint16_t length);
void heap_unsteal(uint16_t length);

#endif