
#pragma once
#ifndef SCR_TREF_H
#define SCR_TREF_H

#include <stdlib.h>
#include <string.h>

bool Compare_strings(char *s1, char *s2);
bool Compare_strings(const char s1[], char *s2);
bool Compare_strings(char *s1, const char s2[]);
bool Compare_strings(const char s1[], const char s2[]);
int String_to_int(char*s);
float String_to_float(char *s);
void Copy_str(char *s1, char *s2);


#endif