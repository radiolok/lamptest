#ifndef UTIL_H
#define UTIL_H

#include "definitions.h"

void cstr2rs(const char *q);
void char2rs(unsigned char data);

void setTxen(const unsigned char state);
unsigned char getTxen(void);

void int2asc(unsigned int liczba, unsigned char* ascii);

void fp2ascii(unsigned int num,
                unsigned char integral,
                unsigned char frac,
                unsigned char *to);
#endif
