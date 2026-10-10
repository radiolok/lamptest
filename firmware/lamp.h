#ifndef LAMP_H
#define LAMP_H

#include <stdint.h>

// One tube record. The layout is the EEPROM format of the user table, so it
// must not change.
typedef struct
{
    uint8_t  name[9];   // TTTTTT s e m: type, socket, electrode system, warm-up
    uint8_t  uhdef;     // heater voltage, 0.1 V
    uint8_t  ihdef;     // heater current, 10 mA (series heaters, uhdef = 0)
    uint8_t  ug1def;    // grid bias magnitude, 0.1 V
    uint16_t uadef;     // anode voltage, 1 V
    uint16_t iadef;     // expected anode current, 0.01 mA
    uint16_t ug2def;    // screen voltage, 1 V
    uint16_t ig2def;    // expected screen current, 0.01 mA
    uint16_t sdef;      // expected S, 0.1 mA/V
    uint16_t rdef;      // expected R, 0.1 kOhm
    uint16_t kdef;      // expected K, 0.1
} lamp_t;

#define LAMP_FLASH_COUNT    81      // read-only slots 0..80
#define LAMP_USER_COUNT     19      // EEPROM slots 81..99
#define LAMP_COUNT          (LAMP_FLASH_COUNT + LAMP_USER_COUNT)
#define LAMP_SUPPLY         0       // slot 0: bench power supply mode
#define LAMP_FIRST_TUBE     2       // slot 1 is reserved

// Name character positions
#define NAME_SOCKET     6
#define NAME_SECTION    7
#define NAME_WARMUP     8

// Characters a user name can hold. User records store indexes into this
// table, flash records store plain ASCII.
#define AZ_COUNT    37
#define AZ_DIGIT0   27              // index of '0'
extern const char AZ[AZ_COUNT];

// Fields of a record, in encoder order. The cursor ("field") walks these.
enum field {
    F_LAMP = 0,         // slot number
    F_NAME = 1,         // name[0..5], fields 1..6
    F_SOCKET = 7,       // name[6], A..J
    F_SECTION = 8,      // name[7], 0..2
    F_WARMUP = 9,       // name[8], 1..9 min
    F_UG1 = 10,
    F_UH,
    F_IH,
    F_UA,
    F_IA,
    F_UG2,
    F_IG2,
    F_S,
    F_R,
    F_K,
    F_COUNT
};

// Electrical parameters, F_UG1..F_K
#define PARAM_COUNT     (F_COUNT - F_UG1)
#define PARAM(f)        ((f) - F_UG1)

static inline uint8_t lamp_is_user(uint8_t num)
{
    return num >= LAMP_FLASH_COUNT;
}

// Display character of name[i]
char lamp_name_char(uint8_t num, const lamp_t *lamp, uint8_t i);

// Electrode system: 0 single tube, 1 or 2 section of a twin tube
uint8_t lamp_section(const lamp_t *lamp);

// Heater warm-up time in sequencer steps
uint16_t lamp_warmup_steps(uint8_t num, const lamp_t *lamp);

// Generic access to the editable fields F_NAME..F_K
uint16_t lamp_field_get(const lamp_t *lamp, uint8_t field);
void     lamp_field_set(lamp_t *lamp, uint8_t field, uint16_t value);
uint8_t  lamp_field_offset(uint8_t field);
uint8_t  lamp_field_is_word(uint8_t field);

// Edit limits of a field for the given slot
void lamp_field_range(uint8_t num, uint8_t field, uint16_t *min, uint16_t *max);

#endif
