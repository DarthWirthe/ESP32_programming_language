
#include "scr_tref.h"

bool Compare_strings(char *s1, char *s2)
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

bool Compare_strings(const char s1[], char *s2)
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

bool Compare_strings(char *s1, const char s2[])
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

bool Compare_strings(const char s1[], const char s2[])
{
	for(int n = 0;;n++)
	{
		if(s1[n]!=s2[n])
			return false;
		if(s1[n]=='\0'&&s2[n]=='\0')
			return true;
	}
}

void Copy_str(char *s1, char *s2)
{
	for (int i = 0; i < 80; i++){
		s2[i] = s1[i];
		if (s1[i] == '\0')
			return;
	}
}

void Copy_str(const char *s1, char *s2)
{
	for (int i = 0; i < 80; i++){
		s2[i] = s1[i];
		if (s1[i] == '\0')
			return;
	}
}

int Const_char_to_int(const char* s)
{
	return atoi(s);
}

float Const_char_to_float(const char* s)
{
	return atof(s);
}
