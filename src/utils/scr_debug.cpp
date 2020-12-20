
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <HardwareSerial.h>

#include "scr_debug.h"

size_t WRITE(uint8_t *buffer, int size) {
    size_t n = 0;
    while(size--) {
        n += Serial.write(*buffer);
        buffer++;
    }
    return n;
}

size_t PRINTF(const char *format, ...)
{
    int len = 0;
    char loc_buf[64];
    char *temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
        return 0;
    };
    if(len >= sizeof(loc_buf)){
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
            return 0;
        }
        len = vsnprintf(temp, len+1, format, arg);
    }
    va_end(arg);
    len = WRITE((uint8_t*)temp, len);
    if(temp != loc_buf){
        free(temp);
    }
    return len;
}
