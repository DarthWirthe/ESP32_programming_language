/*
** 
*/

#include "scr_heap.h"
#include "HardwareSerial.h"

static inline void heap_memcpy_up(uint8_t *dst, uint8_t *src, uint16_t len) {
	dst += len;
	src += len;
	while(len--) *--dst = *--src;
}

static inline void h_memcpy(void *dst, void *src, uint16_t len) {
	uint8_t *dst8 = (uint8_t*)dst;
	uint8_t *src8 = (uint8_t*)src;
	while(len--)
		*dst8++ = *src8++;
}

// Инициализация

Heap::Heap(int _length) {
	heaphdr_t *h;
	heap = new heap_t[_length];
	base = 0;
	length = _length;
	// Один пустой блок занимает всё пространство
	h = (heaphdr_t*)&heap[0];
	h->id = HEAP_ID_FREE;
	h->len = _length - sizeof(heaphdr_t);
}

uint16_t Heap::get_base(){return base;}
heap_t* Heap::get_heap(){return heap;}
uint16_t Heap::get_length(){return length;}

// Поиск блока с идентификатором

static inline heaphdr_t* heap_search(heap_t *heap, uint16_t base, uint16_t length, heap_id_t id) {
	uint16_t current = base;
	while (current < length) {
		heaphdr_t *h = (heaphdr_t*)&heap[current];
		if(h->id == id) return h;
		current += h->len + sizeof(heaphdr_t);
	}
	return 0;
}

// Поиск свободного блока

static inline heap_id_t heap_new_id(heap_t *heap, uint16_t base, uint16_t length) {
	heap_id_t id;
	for (id = 1; id; id++) 
		if (!heap_search(heap, base, length, id))
			return id;
	return 0;
}

// Выделение памяти по идентификатору

static inline bool heap_alloc_internal(heap_t *heap, uint16_t base, uint16_t length, 
heap_id_t id, uint16_t size) {
	uint16_t required = size + sizeof(heaphdr_t);
	heaphdr_t *h = (heaphdr_t*)&heap[base];
	if (h->len >= required) { // если есть свободное пространство
		h->len -= required; // уменьшение свободного блока
		h = (heaphdr_t*)&heap[base + sizeof(heaphdr_t) + h->len];
		h->id = id;
		h->len = size;
		// заполнение нулями
		uint8_t *ptr = (uint8_t*)(h + 1);
		while (size--)
			*ptr++ = 0;
		return true;
	} else {
		// ошибка: нет свободного пространства
	}
	return false;
}

void Heap::heap_dmp() {
	for (int i = 0; i < length; i++) {
		Serial.println(heap[length-i]);
	}
}

// Создание блока

heap_id_t Heap::alloc(uint16_t size) {
	heap_id_t id = heap_new_id(heap, base, length);
	if (!id) {
		/*Ошибка!*/
	}
	if (!heap_alloc_internal(heap, base, length, id, size)) {
		/*Ошибка!*/
	}
	return id;
}

// Удаление блока

void Heap::free(heap_id_t id) {
	heaphdr_t *h = heap_search(heap, base, length, id);
	if (h) {
		uint16_t len = h->len + sizeof(heaphdr_t);
		heap_memcpy_up(heap + base + len, heap + base, ((uint8_t*)h - (uint8_t*)&heap[0]) - base);
		h = (heaphdr_t*)&heap[base]; // увеличение свободного блока
		h->len += len;
	} else {
		Serial.print("Not found id:");
		Serial.println(id);
	}
}

// Увеличение размера блока

void Heap::realloc(heap_id_t id, uint16_t size) {
	heaphdr_t *h_old, *h_new;
	// заголовок старого блока
	h_old = heap_search(heap, base, length, id);
	// создание нового блока
	if (!heap_alloc_internal(heap, base, length, id, h_old->len))
	{/*Ошибка!*/}
	h_new = heap_search(heap, base, length, id);
	h_memcpy(h_new + 1, h_old + 1, h_old->len);
	// удаляет старый блок
	free(h_old->id);
}

// Возвращает длину блока

uint16_t Heap::get_length(heap_id_t id) {
  heaphdr_t *h = heap_search(heap, base, length, id);
  if (h==nullptr){/*Ошибка!*/}
  return h->len;
}

// Возвращает адрес данных в блоке

void* Heap::get_addr(heap_id_t id) {
	heaphdr_t *h = heap_search(heap, base, length, id);
	if (h==nullptr){/*Ошибка!*/}
	return h + 1;
}

// Возвращает размер свободного блока

uint16_t Heap::get_free_size() {
	return ((heaphdr_t*)&heap[base])->len;
}

// Возвращает указатель на начало свободного блока

heap_t* Heap::get_relative_base() {
	return heap + base;
}

// Занимает [length] байт из кучи (для стека)

void Heap::mem_steal(uint16_t length) {
	heaphdr_t *h = (heaphdr_t*)&heap[base];
	uint16_t len;
	if (h->id != HEAP_ID_FREE) {
		/*Ошибка! Куча повреждена*/
	}
	len = h->len;
	if (len < length) {
		/*Ошибка! Переполнение*/
	}
	base += length;
	h = (heaphdr_t*)&heap[base];
	h->id = HEAP_ID_FREE;
	h->len = len - length;
}

// Возвращает [length] байт в кучу

void Heap::mem_unsteal(uint16_t length) {
	heaphdr_t *h = (heaphdr_t*)&heap[base];
	uint16_t len;
	if (h->id != HEAP_ID_FREE) {
		/*Ошибка! Куча повреждена*/
	}
	if (base < length) {
		/*Ошибка! Стек за пределами кучи*/
	}
	len = h->len;
	base -= length;
	h = (heaphdr_t*)&heap[base];
	h->id = HEAP_ID_FREE;
	h->len = len + length;
}
  
