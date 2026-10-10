#ifndef FORMAT_H
#define FORMAT_H

#include <stdint.h>

// Four least significant decimal digits of num, least significant first
void fmt_digits(uint16_t num, char digits[4]);

// Writes num as fixed point: <integral> digits, '.', <frac> digits
// (no '.' when frac is 0). Leading zeros of the integral part are blanked,
// the last one is kept. integral + frac must not exceed 4.
// The output is not null-terminated.
void fmt_fixed(uint16_t num, uint8_t integral, uint8_t frac, char *to);

#endif
