
#include "scr_heap.h"
#include "HardwareSerial.h"

uint8_t heap[DEFAULT_HEAP_SIZE];
uint16_t heap_base = 0;

// Заголовок блока

typedef struct {
	heap_id_t id; // 8 / 16 бит
	uint16_t len; // 16 бит
} heap_t;

void heap_memcpy_up(uint8_t *dst, uint8_t *src, uint16_t len) {
	dst += len;  src += len;
	while(len--) *--dst = *--src;
}

static inline void h_memcpy(void *dst, void *src, uint16_t len) {
	uint8_t *dst8 = (uint8_t*)dst;
	uint8_t *src8 = (uint8_t*)src;
	while(len--)
		*dst8++ = *src8++;
}

// Поиск блока с идентификатором

heap_t* heap_search(heap_id_t id) {
	uint16_t current = heap_base;
	while(current < sizeof(heap)) {
		heap_t *h = (heap_t*)&heap[current];
		if(h->id == id) return h;
		current += h->len + sizeof(heap_t);
	}
	return 0;
}

// Поиск свободного блока

heap_id_t heap_new_id() {
	heap_id_t id;
	for (id = 1; id; id++) 
		if (!heap_search(id))
			return id;
	return 0;
}

// Выделение памяти по идентификатору

bool heap_alloc_internal(heap_id_t id, uint16_t size) {
	uint16_t required = size + sizeof(heap_t);
	heap_t *h = (heap_t*)&heap[heap_base];
	if(h->len >= required) { // если есть свободное пространство
		h->len -= required; // уменьшение свободного блока
		h = (heap_t*)&heap[heap_base + sizeof(heap_t) + h->len];
		h->id = id;
		h->len = size;
		// заполнение нулями
		uint8_t *ptr = (uint8_t*)(h + 1);
		while(size--)
			*ptr++ = 0;
		return true;
	}
	return false;
}

void heap_dbg() {
	for(int i = 0; i < DEFAULT_HEAP_SIZE; i++){
		Serial.println(heap[DEFAULT_HEAP_SIZE-i]);
	}
}

// Создание блока

heap_id_t heap_alloc(uint16_t size) {
	heap_id_t id = heap_new_id();
	if(!id){
		/*Ошибка!*/
	}
	if(!heap_alloc_internal(id, size)){
		/*Ошибка!*/
	}
	return id;
}

// Удаление блока

void heap_free(heap_id_t id) {
	heap_t *h = heap_search(id);
	if(h){
		uint16_t len = h->len + sizeof(heap_t);
		heap_memcpy_up(heap + heap_base + len, heap + heap_base, ((uint8_t*)h - (uint8_t*)&heap[0]) - heap_base);
		h = (heap_t*)&heap[heap_base]; // увеличение свободного блока
		h->len += len;
	} else {
		Serial.print("Not found id:");
		Serial.println(id);
	}
}

// Увеличение размера блока

void heap_realloc(heap_id_t id, uint16_t size) {
	heap_t *h_old, *h_new;
	// заголовок старого блока
	h_old = heap_search(id);
	// создание нового блока
	if(!heap_alloc_internal(id, h_old->len))
	{/*Ошибка!*/}
	h_new = heap_search(id);
	h_memcpy(h_new + 1, h_old + 1, h_old->len);
	// удаляет старый блок
	heap_free(h_old->id);
}

// Возвращает длину блока

uint16_t heap_get_length(heap_id_t id) {
  heap_t *h = heap_search(id);
  if(h==nullptr){/*Ошибка!*/}
  return h->len;
}

// Возвращает адрес данных в блоке

void* heap_get_addr(heap_id_t id) {
	heap_t *h = heap_search(id);
	if(h==nullptr){/*Ошибка!*/}
	return h + 1;
}

// Инициализация

void heap_init() {
	heap_t *h;
	// Один пустой блок занимает всё пространство
	h = (heap_t*)&heap[0];
	h->id = HEAP_ID_FREE;
	h->len = sizeof(heap) - sizeof(heap_t);
}

// Возвращает размер свободного блока

uint16_t heap_get_free_size() {
	return ((heap_t*)&heap[heap_base])->len;
}

// Возвращает указатель на начало кучи

uint8_t* heap_get_base() {
	return heap;
}

// Возвращает указатель на начало свободного блока

uint8_t* heap_get_relative_base() {
	return heap + heap_base;
}

// Занимает [length] байт из кучи (для стека)

void heap_steal(uint16_t length) {
	heap_t *h = (heap_t*)&heap[heap_base];
	uint16_t len;
	
	if(h->id != HEAP_ID_FREE) {
		/*Ошибка!*/
	}
	
	len = h->len;
	if(len < length) {
		/*Ошибка!*/
	}

	heap_base += length;
	h = (heap_t*)&heap[heap_base];
	h->id = HEAP_ID_FREE;
	h->len = len - length;
}

// Возвращает [length] байт в кучу

void heap_unsteal(uint16_t length) {
	heap_t *h = (heap_t*)&heap[heap_base];
	uint16_t len;

	if(h->id != HEAP_ID_FREE) {
		/*Ошибка!*/
	}

	if(heap_base < length) {
		/*Ошибка!*/
	}

	len = h->len;
	heap_base -= length;
	h = (heap_t*)&heap[heap_base];
	h->id = HEAP_ID_FREE;
	h->len = len + length;
}
  