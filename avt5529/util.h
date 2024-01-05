#ifndef UTIL_H
#define UTIL_H

#include "definitions.h"

unsigned char temp_str[5];

void cstr2rs(const char *q);
void char2rs(unsigned char data);

void int2asc(unsigned int liczba, unsigned char* ascii);

void fp2ascii(unsigned int num,
                unsigned char integral,
                unsigned char frac,
                unsigned char *to);
#endif