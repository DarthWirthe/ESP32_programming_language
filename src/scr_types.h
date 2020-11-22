
#pragma once
#ifndef SCR_TYPES_H
#define SCR_TYPES_H

#include <stdint.h> // для фиксированных типов

#define IMMEDIATE_MASK_16 0x8000 // маска для знаковых типов 16 бит
#define IMMEDIATE_MASK_32 0x80000000 // маска для знаковых типов 32 бит

typedef int8_t  char_t;
typedef int16_t short_t;
typedef int32_t int_t;

typedef uint8_t  heap_t;
typedef uint32_t stack_t;

typedef union {
  char  b[4];
  short s[2];
  int   i[1];
  float f[1];
} union_t;

typedef union {
  int   i;
  float f;
} num_t;



#endif
