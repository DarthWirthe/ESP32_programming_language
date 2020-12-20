/* scr_heap.cpp
** Виртуальная память
*/

#include "scr_heap.h"
#include "utils/scr_debug.h"
#include "HardwareSerial.h"

// Макрос для ошибок

#define HEAP_ERR(err) throw (err)

// Макрос для отладки

#define H_DBG

#ifdef H_DBG
	#define DEBUGF(__f, ...) __DEBUGF__(__f, ##__VA_ARGS__)
#else
	#define DEBUGF(__f, ...)
#endif // H_DBG

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

Heap::Heap(int size) {
	heaphdr_t *h;
	heap = new heap_t[size];
	//memset(heap, 0x00, sizeof(heap_t) * size);
	base = 0;
	length = size;
	// Один пустой блок занимает всё пространство
	h = (heaphdr_t*)&heap[0];
	h->id = HEAP_ID_FREE;
	h->len = size - sizeof(heaphdr_t);
}

// Поиск блока с идентификатором

static inline heaphdr_t* heap_search(heap_t *heap, uint16_t base, uint16_t length, u16_t id) {
	uint16_t current = base;
	while (current < length) {
		heaphdr_t *h = (heaphdr_t*)&heap[current];
		if((h->id & HEAP_ID_MASK) == id) return h;
		current += h->len + sizeof(heaphdr_t);
	}
	return 0;
}

// Поиск свободного блока

u16_t Heap::heap_new_id() {
	u16_t id;
	for (id = 1; id; id++) 
		if (!heap_search(heap, base, length, id))
			return id;
	return 0;
}

// Выделение памяти по идентификатору

bool Heap::heap_alloc_internal(u16_t id, uint16_t size) {
	uint16_t required = size + sizeof(heaphdr_t);
	heaphdr_t *h = (heaphdr_t*)&heap[base];
	if (h->len >= required) { // если есть свободное пространство
		h->len -= required; // уменьшение свободного блока
		h = (heaphdr_t*)&heap[base + sizeof(heaphdr_t) + h->len];
		h->id = id & HEAP_ID_MASK;
		h->len = size;
		// заполнение нулями
		uint8_t *ptr = (uint8_t*)(h + 1);
		while (size--)
			*ptr++ = 0;
		return true;
	} else {
		HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	}
	return false;
}

void Heap::heap_dmp() {
	for (int i = 0; i < length; i++) {
		PRINTF("[%d][%u]\n", i, heap[length-i]);
	}
}

// Создание блока

u16_t Heap::alloc(uint16_t size) {
	u16_t id = heap_new_id();
	if (!id) {
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	}
	if (!heap_alloc_internal(id, size)) {
		// Если нет свободного пространства:
		garbage_collect(); // сборка мусора
		id = heap_new_id(); // ид был переопределён
		if (!heap_alloc_internal(id, size)) // попробовать снова
			HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	}
	return id;
}

// Удаление блока

void Heap::free(u16_t id) {
	heaphdr_t *h = heap_search(heap, base, length, id);
	if (h) {
		uint16_t len = h->len + sizeof(heaphdr_t);
		heap_memcpy_up(heap + base + len, heap + base, ((uint8_t*)h - (uint8_t*)&heap[0]) - base);
		h = (heaphdr_t*)&heap[base]; // увеличение свободного блока
		h->len += len;
	} else {
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	}
}

// Увеличение размера блока

void Heap::realloc(u16_t id, uint16_t size) {
	heaphdr_t *h, *h_new;
	// заголовок старого блока
	h = (heaphdr_t*)&heap[base];
	if(h->len >= size + sizeof(heaphdr_t))
		HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	h = heap_search(heap, base, length, id);
	// создание нового блока
	if (!heap_alloc_internal(id, size))
		HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	h_new = heap_search(heap, base, length, id);
	h_memcpy(h_new + 1, h + 1, h->len);
	// Старый блок будет удалён сборщиком
	h->id = HEAP_ID_FREE;
}

// Возвращает длину блока

uint16_t Heap::get_length(u16_t id) {
	heaphdr_t *h = heap_search(heap, base, length, id);
	if (!h) {
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	}
	return h->len;
}

// Возвращает адрес данных в блоке

void* Heap::get_addr(u16_t id) {
	heaphdr_t *h = heap_search(heap, base, length, id);
	if (!h)
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
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

bool heap_id_in_use(uint16_t id){
	return true;
}

/* Функция сборки мусора
 * Проверяет каждый объект в куче на достижимость,
 * если объект не используется, то он удаляется из памяти
*/

void Heap::garbage_collect() {
	uint16_t current = base;
	uint16_t len;
	
	while (current < length) {
		heaphdr_t *h = (heaphdr_t*)&heap[current];
		len = h->len + sizeof(heaphdr_t);
		if (!heap_id_in_use(h->id & HEAP_ID_MASK)) {
			// дефрагментация!
			heap_memcpy_up(heap + base + len, heap + base, current - base);
			// добавить освобождённую память в свободный блок
			((heaphdr_t*)&heap[base])->len += len;
		}
		current += len;
	}
	// Ошибка, если какой-то блок был повреждён
	if(current != this->length) {
		HEAP_ERR(HEAP_ERR_CORRUPTED);
	}
}

// Занимает [length] байт из кучи (для стека)

void Heap::mem_steal(uint16_t length) {
	heaphdr_t *h = (heaphdr_t*)&heap[base];
	uint16_t len;
	if (h->id != HEAP_ID_FREE)
		HEAP_ERR(HEAP_ERR_CORRUPTED); /* Ошибка! Куча повреждена */
	len = h->len;
	if (len < length)
		HEAP_ERR(HEAP_ERR_OVERFLOW); /* Ошибка! Переполнение */
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
		HEAP_ERR(HEAP_ERR_CORRUPTED); /* Ошибка! Куча повреждена */
	}
	if (base < length) {
		HEAP_ERR(HEAP_ERR_UNDERFLOW); /* Ошибка! Стек пуст */
	}
	len = h->len;
	base -= length;
	h = (heaphdr_t*)&heap[base];
	h->id = HEAP_ID_FREE;
	h->len = len + length;
}

void Heap::delete_heap() {
	delete[] heap;
}
