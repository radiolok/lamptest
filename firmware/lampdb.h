#ifndef LAMPDB_H
#define LAMPDB_H

#include "lamp.h"

// Tube database: read-only records in flash (slots 0..80) and user
// records in EEPROM (slots 81..99).

void lampdb_load(uint8_t num, lamp_t *lamp);

// Re-read name[0..5] and the socket letter of a user record
void lampdb_reload_name(uint8_t num, lamp_t *lamp);

// Store one field (F_NAME..F_K) of a user record
void lampdb_save_field(uint8_t num, const lamp_t *lamp, uint8_t field);

// Slot selected at power-up
uint8_t lampdb_last(void);
void    lampdb_set_last(uint8_t num);

#endif
