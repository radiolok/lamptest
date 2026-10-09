// Host stand-in for avr-libc <avr/pgmspace.h>: flash is ordinary memory
#ifndef FAKE_AVR_PGMSPACE_H
#define FAKE_AVR_PGMSPACE_H

#include <stdint.h>
#include <string.h>

#define PROGMEM
#define pgm_read_byte(p)        (*(const uint8_t *)(p))
#define memcpy_P(dst, src, n)   memcpy((dst), (src), (n))

#endif
