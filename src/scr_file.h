
#pragma once
#ifndef SCR_FILE_H
#define SCR_FILE_H

#include <stdint.h>
#include "FS.h"
#include "SPIFFS.h"
#include "crc32/ErriezCRC32.h" // библиотека Си

#define FORMAT_SPIFFS_IF_FAILED true

bool SPIFFS_BEGIN(void);
File FS_OPEN_FILE(fs::FS &fs, const char *path);
void SPIFFS_READ_FILE(fs::FS &fs, const char *path); //SPIFFS_READ_FILE(SPIFFS, "/hello.txt");


#endif