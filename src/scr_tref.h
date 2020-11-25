
#pragma once
#ifndef SCR_TREF_H
#define SCR_TREF_H

#include <stdlib.h>
#include <string.h>

bool Compare_strings(char *s1, char *s2);
bool Compare_strings(const char s1[], char *s2);
bool Compare_strings(char *s1, const char s2[]);
bool Compare_strings(const char s1[], const char s2[]);
int Const_char_to_int(const char*s);
float Const_char_to_float(const char *s);
void Copy_str(char *s1, char *s2);
void Copy_str(const char *s1, char *s2);


#endif
