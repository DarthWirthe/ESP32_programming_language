
#pragma once
#ifndef SCR_DEBUG_H
#define SCR_DEBUG_H

#include <cstddef>

#define __DEBUGF__(__f, ...) PRINTF(__f, ##__VA_ARGS__)

size_t WRITE(uint8_t *buffer, int size);
size_t PRINTF(const char *format, ...);

#endif
