// Host stand-in for avr-libc <avr/eeprom.h>: EEPROM is ordinary memory.
// Writes are counted so tests can check that nothing was stored.
#ifndef FAKE_AVR_EEPROM_H
#define FAKE_AVR_EEPROM_H

#include <stdint.h>
#include <string.h>

#define EEMEM

extern unsigned fake_eeprom_writes;

static inline uint8_t eeprom_read_byte(const uint8_t *p)
{
    return *p;
}

static inline void eeprom_read_block(void *dst, const void *src, size_t n)
{
    memcpy(dst, src, n);
}

static inline void eeprom_update_byte(uint8_t *p, uint8_t v)
{
    if (*p != v) {
        *p = v;
        fake_eeprom_writes++;
    }
}

static inline void eeprom_update_word(uint16_t *p, uint16_t v)
{
    if (*p != v) {
        *p = v;
        fake_eeprom_writes++;
    }
}

#endif
